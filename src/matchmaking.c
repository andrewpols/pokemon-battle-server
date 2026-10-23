#include <stdlib.h>
#include <stdio.h>
#include "matchmaking.h"
#include "client_server.h"
#include "game.h"
#include "log.h"

DLL *add_match(DLL **match_list_ptr) {
  Match *new_match = malloc(sizeof(Match));
  new_match->num_ready = 0;
  new_match->rematch_votes = 0;
  new_match->client_list = NULL;
  add_node(match_list_ptr, new_match);
  return *match_list_ptr;
}

int matchmake(DLL **match_list_ptr, Client *client) {
  DLL *match_node = find_lobby(*match_list_ptr);
  if (match_node == NULL) {
    match_node = add_match(match_list_ptr);
    if (match_node == NULL) {
      log_msg("Error matchmaking. Please try again.");
      return -1;
    }
  }
  Match *match = (Match *)(match_node->data);
  add_node(&(match->client_list), client);
  client->match_node = match->client_list;
  client->match = match;
  match->num_ready++;
  if (match->num_ready == 2) {
    Client *client1 = (Client *)(match->client_list->data);
    Client *client2 = (Client *)(match->client_list->next->data);
    log_msg("Match found: %s vs. %s", client1->username, client2->username);
    print_users_in_match(match);
    DLL *curr = match->client_list;
    while (curr != NULL) {
      Client *c = (Client *)(curr->data);
      c->status = IN_MATCH;
      curr = curr->next;
    }
    send_match_start(match);
    return 1;
  }
  client->status = IN_LOBBY;
  send_to_client(client, "=== Welcome to Pokemon Battle! ===\nWaiting for an opponent...");
  return 0;
}

DLL *find_lobby(DLL *match_list) {
  DLL *curr = match_list;
  while (curr != NULL) {
    Match *match = (Match *)(curr->data);
    if (match->num_ready < 2 && match->num_ready > 0)
      return curr;
    curr = curr->next;
  }
  return NULL;
}

static void free_match_from_list(Match *match, DLL **match_list_ptr) {
  DLL *curr = *match_list_ptr;
  while (curr != NULL) {
    if ((Match *)curr->data == match) {
      remove_node(match_list_ptr, curr);
      break;
    }
    curr = curr->next;
  }
  free(match);
}

void remove_user_from_match(Client *client, DLL **match_list_ptr) {
  Match *match = client->match;
  if (match == NULL)
    return;

  remove_node(&(match->client_list), client->match_node);
  client->match = NULL;
  client->match_node = NULL;
  match->num_ready--;

  if (match->num_ready == 0 || match->client_list == NULL) {
    free_match_from_list(match, match_list_ptr);
    return;
  }

  Client *other = (Client *)(match->client_list->data);
  other->match = NULL;
  other->match_node = NULL;
  free_match_from_list(match, match_list_ptr);
  other->status = IN_LOBBY;
  send_to_client(other, "Your opponent disconnected! Waiting for a new opponent...");
  matchmake(match_list_ptr, other);
}

void print_users_in_match(Match *match) {
  Client *client1 = (Client *)(match->client_list->data);
  Client *client2 = (Client *)(match->client_list->next->data);
  char payload[MAX_CLIENT_MSG_SIZE];
  snprintf(payload, MAX_CLIENT_MSG_SIZE, "Match found!\n\t%s vs. %s", client1->username, client2->username);
  Packet *pkt = make_packet(CMD_CHAT, payload);
  broadcast_packet(match, pkt);
  free(pkt);
}

void broadcast_packet(Match *match, Packet *pkt) {
  DLL *curr = match->client_list;
  uint32_t net_len = htonl(pkt->payload_length);
  while (curr != NULL) {
    Client *client = (Client *)(curr->data);
    write(client->fd, &(pkt->cmd_type), sizeof(uint8_t));
    write(client->fd, &net_len, sizeof(uint32_t));
    write(client->fd, pkt->payload, pkt->payload_length);
    curr = curr->next;
  }
}
