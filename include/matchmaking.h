#ifndef _MATCHMAKING_H_
#define _MATCHMAKING_H_

#define CLIENT_BUF_SIZE 256
#define MAX_CLIENT_MSG_SIZE 128

#include <unistd.h>
#include <netinet/in.h>
#include <string.h>

#include "client_server.h"
#include "doubly_linked_list.h"
#include "pokemon.h"

typedef enum {
  IN_LOBBY,
  IN_MATCH,
  MOVE_PLAYED,
  REMATCH_PENDING,
  DISCONNECTING
} client_status;

typedef struct {
  int num_ready;
  int rematch_votes;
  DLL *client_list;
} Match;


typedef struct client {
  int fd;
  char *username;
  client_status status;
  Match *match;
  DLL *match_node;
  Party party;
  int chosen_move;

  char buf[CLIENT_BUF_SIZE];
  int buf_len;
  struct in_addr ipaddr;
  uint16_t port;
} Client;

void print_users_in_match(Match *match);
DLL *add_match(DLL **match_list_ptr);
int matchmake(DLL **match_list_ptr, Client *client);
DLL *find_lobby(DLL *match_list);
void remove_user_from_match(Client *client, DLL **match_list_ptr);
void broadcast_packet(Match *match, Packet *pkt);
#endif
