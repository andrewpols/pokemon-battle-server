#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "matchmaking.h"
#include "pokemon.h"
#include "game.h"

#define MAX_MSG 512

void send_to_client(Client *client, const char *msg) {
  Packet *pkt = make_packet(CMD_CHAT, msg);
  uint32_t net_len = htonl(pkt->payload_length);
  write(client->fd, &pkt->cmd_type, sizeof(uint8_t));
  write(client->fd, &net_len, sizeof(uint32_t));
  write(client->fd, pkt->payload, pkt->payload_length);
  free(pkt);
}

static Client *get_opponent(Client *client) {
  if (client->match == NULL)
    return NULL;
  DLL *curr = client->match->client_list;
  while (curr != NULL) {
    Client *c = (Client *)curr->data;
    if (c != client)
      return c;
    curr = curr->next;
  }
  return NULL;
}

static void send_party_stats(Client *viewer, Client *target) {
  char msg[MAX_MSG * 4] = { '\0' };
  char line[MAX_MSG];

  snprintf(msg, sizeof(msg), "=== %s's Party ===\n", target->username);
  for (int i = 0; i < PARTY_SIZE; i++) {
    Pokemon *p = &target->party.party[i];
    snprintf(line, MAX_MSG, "%s%d. %s (HP: %d/%d ATK: %d DEF: %d SPD: %d)%s\n", i == target->party.active ? "* " : "  ", i + 1, p->name, p->hp, p->max_hp, p->attack, p->defense, p->speed, p->hp == 0 ? " [FAINTED]" : "");
    strncat(msg, line, sizeof(msg) - strlen(msg) - 1);
    if (target == viewer) {
      for (int j = 0; j < NUM_MOVES; j++) {
        snprintf(line, MAX_MSG, "     %d. %s (PWR: %d)\n", j + 1, p->moves[j].name, p->moves[j].power);
        strncat(msg, line, sizeof(msg) - strlen(msg) - 1);
      }
    }
  }
  send_to_client(viewer, msg);
}

static void send_instructions(Client *client) {
  char msg[MAX_MSG * 4];
  Pokemon *active = &client->party.party[client->party.active];
  snprintf(msg, sizeof(msg), "=== Battle Start! ===\n"
                             "Commands:\n"
                             "  move <1-4>   - Use a move\n"
                             "  switch <1-4> - Switch pokemon\n"
                             "  stats        - View both parties\n"
                             "  chat <msg>   - Send a message\n"
                             "  q            - Forfeit\n\n"
                             "Your active pokemon: %s (HP: %d/%d)\n"
                             "Moves:\n"
                             "  1. %s (PWR: %d)\n"
                             "  2. %s (PWR: %d)\n"
                             "  3. %s (PWR: %d)\n"
                             "  4. %s (PWR: %d)\n",
           active->name,
           active->hp,
           active->max_hp,
           active->moves[0].name,
           active->moves[0].power,
           active->moves[1].name,
           active->moves[1].power,
           active->moves[2].name,
           active->moves[2].power,
           active->moves[3].name,
           active->moves[3].power);
  send_to_client(client, msg);
}

void send_match_start(Match *match) {
  DLL *curr = match->client_list;
  while (curr != NULL) {
    send_instructions((Client *)curr->data);
    curr = curr->next;
  }
}

static void end_match(Match *match, Client *winner, Client *loser, int forfeit, DLL **match_list_ptr) {
  char msg[MAX_MSG];
  if (forfeit) {
    snprintf(msg, MAX_MSG, "%s has forfeited! %s wins!", loser->username, winner->username);
  } else {
    snprintf(msg, MAX_MSG, "%s has no pokemon left! %s wins!", loser->username, winner->username);
  }
  Packet *pkt = make_packet(CMD_CHAT, msg);
  broadcast_packet(match, pkt);
  free(pkt);

  send_to_client(winner, "Rematch? Type 'rematch yes' or 'rematch no'");
  send_to_client(loser, "Rematch? Type 'rematch yes' or 'rematch no'");

  match->rematch_votes = 0;
  winner->status = REMATCH_PENDING;
  loser->status = REMATCH_PENDING;
}

static void resolve_turn(Match *match, DLL **match_list_ptr) {
  DLL *curr = match->client_list;
  Client *c1 = (Client *)curr->data;
  Client *c2 = (Client *)curr->next->data;

  Pokemon *p1 = &c1->party.party[c1->party.active];
  Pokemon *p2 = &c2->party.party[c2->party.active];

  Client *first = (p1->speed >= p2->speed) ? c1 : c2;
  Client *second = (first == c1) ? c2 : c1;
  Pokemon *pFirst = &first->party.party[first->party.active];
  Pokemon *pSecond = &second->party.party[second->party.active];

  char msg[MAX_MSG];
  Packet *pkt;

  if (first->chosen_move != -1) {
    Move *move1 = &pFirst->moves[first->chosen_move];
    int dmg1 = calculate_damage(pFirst, pSecond, move1);
    pSecond->hp -= dmg1;
    if (pSecond->hp < 0)
      pSecond->hp = 0;

    snprintf(msg, MAX_MSG, "%s's %s used %s! Dealt %d damage! (%s HP: %d/%d)", first->username, pFirst->name, move1->name, dmg1, pSecond->name, pSecond->hp, pSecond->max_hp);
    pkt = make_packet(CMD_CHAT, msg);
    broadcast_packet(match, pkt);
    free(pkt);

    if (pSecond->hp == 0) {
      snprintf(msg, MAX_MSG, "%s's %s fainted!", second->username, pSecond->name);
      pkt = make_packet(CMD_CHAT, msg);
      broadcast_packet(match, pkt);
      free(pkt);

      int found = 0;
      for (int i = 0; i < PARTY_SIZE; i++) {
        if (second->party.party[i].hp > 0) {
          second->party.active = i;
          found = 1;
          snprintf(msg, MAX_MSG, "%s sends out %s!", second->username, second->party.party[i].name);
          pkt = make_packet(CMD_CHAT, msg);
          broadcast_packet(match, pkt);
          free(pkt);
          break;
        }
      }
      if (!found) {
        end_match(match, first, second, 0, match_list_ptr);
        return;
      }
      goto reset_turn;
    }
  }

  if (second->chosen_move != -1) {
    Move *move2 = &pSecond->moves[second->chosen_move];
    int dmg2 = calculate_damage(pSecond, pFirst, move2);
    pFirst->hp -= dmg2;
    if (pFirst->hp < 0)
      pFirst->hp = 0;

    snprintf(msg, MAX_MSG, "%s's %s used %s! Dealt %d damage! (%s HP: %d/%d)", second->username, pSecond->name, move2->name, dmg2, pFirst->name, pFirst->hp, pFirst->max_hp);
    pkt = make_packet(CMD_CHAT, msg);
    broadcast_packet(match, pkt);
    free(pkt);

    if (pFirst->hp == 0) {
      snprintf(msg, MAX_MSG, "%s's %s fainted!", first->username, pFirst->name);
      pkt = make_packet(CMD_CHAT, msg);
      broadcast_packet(match, pkt);
      free(pkt);

      int found = 0;
      for (int i = 0; i < PARTY_SIZE; i++) {
        if (first->party.party[i].hp > 0) {
          first->party.active = i;
          found = 1;
          snprintf(msg, MAX_MSG, "%s sends out %s!", first->username, first->party.party[i].name);
          pkt = make_packet(CMD_CHAT, msg);
          broadcast_packet(match, pkt);
          free(pkt);
          break;
        }
      }
      if (!found) {
        end_match(match, second, first, 0, match_list_ptr);
        return;
      }
    }
  }

reset_turn:
  c1->status = IN_MATCH;
  c2->status = IN_MATCH;
}

void handle_move(Client *client, Packet *pkt, DLL **match_list_ptr) {
  if (client->status == MOVE_PLAYED) {
    send_to_client(client, "You already made your move this turn! Wait for your opponent.");
    return;
  }
  if (client->status != IN_MATCH) {
    send_to_client(client, "You are not in a match!");
    return;
  }

  int move_idx = atoi(pkt->payload) - 1;
  if (move_idx < 0 || move_idx >= NUM_MOVES) {
    send_to_client(client, "Invalid move. Choose 1-4. Use 'stats' to see your moves.");
    return;
  }

  client->chosen_move = move_idx;
  client->status = MOVE_PLAYED;

  char msg[MAX_MSG];
  snprintf(msg, MAX_MSG, "%s chose a move! Waiting for opponent...", client->username);
  send_to_client(client, msg);

  Client *opponent = get_opponent(client);
  if (opponent != NULL) {
    snprintf(msg, MAX_MSG, "%s is choosing their move...", client->username);
    send_to_client(opponent, msg);
    if (opponent->status == MOVE_PLAYED) {
      resolve_turn(client->match, match_list_ptr);
    }
  }
}

void handle_switch(Client *client, Packet *pkt, DLL **match_list_ptr) {
  if (client->status == MOVE_PLAYED) {
    send_to_client(client, "You already made your move this turn!");
    return;
  }
  if (client->status != IN_MATCH) {
    send_to_client(client, "You are not in a match!");
    return;
  }

  int idx = atoi(pkt->payload) - 1;
  if (idx < 0 || idx >= PARTY_SIZE) {
    send_to_client(client, "Invalid pokemon index. Choose 1-4.");
    return;
  }
  if (idx == client->party.active) {
    send_to_client(client, "That pokemon is already active!");
    return;
  }
  if (client->party.party[idx].hp == 0) {
    send_to_client(client, "That pokemon has fainted!");
    return;
  }

  client->party.active = idx;
  client->chosen_move = -1;
  client->status = MOVE_PLAYED;

  char msg[MAX_MSG];
  snprintf(msg, MAX_MSG, "%s switched to %s!", client->username, client->party.party[idx].name);
  Packet *response = make_packet(CMD_CHAT, msg);
  broadcast_packet(client->match, response);
  free(response);

  Client *opponent = get_opponent(client);
  if (opponent && opponent->status == MOVE_PLAYED) {
    resolve_turn(client->match, match_list_ptr);
  }
}

void handle_stats(Client *client, Packet *pkt, DLL **match_list_ptr) {
  (void)pkt;
  (void)match_list_ptr;
  send_party_stats(client, client);
  Client *opponent = get_opponent(client);
  if (opponent)
    send_party_stats(client, opponent);
}

void handle_chat(Client *client, Packet *pkt, DLL **match_list_ptr) {
  (void)match_list_ptr;
  char payload[MAX_MSG];
  snprintf(payload, MAX_MSG, "%s says:\n\t%s", client->username, pkt->payload);
  Packet *response = make_packet(CMD_CHAT, payload);
  broadcast_packet(client->match, response);
  free(response);
}

void handle_quit(Client *client, Packet *pkt, DLL **match_list_ptr) {
  (void)pkt;

  if (client->status == IN_LOBBY || client->match == NULL) {
    send_to_client(client, "Goodbye!");
    client->status = DISCONNECTING;
    return;
  }

  Client *opponent = get_opponent(client);
  if (opponent) {
    end_match(client->match, opponent, client, 1, match_list_ptr);
  } else {
    send_to_client(client, "Goodbye!");
    client->status = DISCONNECTING;
  }
}

void handle_rematch(Client *client, Packet *pkt, DLL **match_list_ptr) {
  if (client->status != REMATCH_PENDING) {
    send_to_client(client, "No rematch pending.");
    return;
  }

  Match *match = client->match;
  Client *opponent = get_opponent(client);

  if (strncmp(pkt->payload, "yes", 3) == 0) {
    match->rematch_votes++;
    send_to_client(client, "Waiting for opponent's decision...");

    if (match->rematch_votes == 2) {
      match->rematch_votes = 0;
      DLL *curr = match->client_list;
      while (curr != NULL) {
        Client *c = (Client *)curr->data;
        assign_random_party(&c->party);
        c->status = IN_MATCH;
        curr = curr->next;
      }
      send_match_start(match);
    }
  } else {
    send_to_client(client, "Goodbye!");
    client->status = DISCONNECTING;

    if (opponent != NULL) {
      match->rematch_votes = 0;
      match->num_ready = 0;
      match->client_list = NULL;
      client->match = NULL;
      opponent->match = NULL;
      opponent->match_node = NULL;
      opponent->status = IN_LOBBY;
      matchmake(match_list_ptr, opponent);
    } else {
      match->num_ready = 0;
      match->client_list = NULL;
      client->match = NULL;
    }
  }
}

typedef void (*Handler)(Client *, Packet *, DLL **);

static Handler dispatch_table[] = {
  (Handler)handle_move,
  (Handler)handle_switch,
  (Handler)handle_stats,
  (Handler)handle_chat,
  (Handler)handle_quit,
  (Handler)handle_rematch
};

void handle_command(Client *client, Packet *pkt, DLL **match_list_ptr) {
  if (pkt->cmd_type == CMD_QUIT) {
    handle_quit(client, pkt, match_list_ptr);
    return;
  }
  if (pkt->cmd_type == CMD_REMATCH) {
    handle_rematch(client, pkt, match_list_ptr);
    return;
  }
  if (client->status == IN_LOBBY || client->status == DISCONNECTING || client->match == NULL) {
    send_to_client(client, "Waiting for an opponent...");
    return;
  }
  if (pkt->cmd_type >= sizeof(dispatch_table) / sizeof(dispatch_table[0]))
    return;

  dispatch_table[(int)pkt->cmd_type](client, pkt, match_list_ptr);
}
