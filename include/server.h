#ifndef _SERVER_H_
#define _SERVER_H_

#ifndef PORT
#define PORT 30000
#endif

#define MAX_CLIENTS 6

#include "client_server.h"
#include "matchmaking.h"
#include "pokemon.h"
#include "game.h"

int add_client(ClientReturn *c, DLL **match_list_ptr, DLL **client_list_ptr, fd_set *allset_ptr, int *max_fd);
int remove_client(DLL *client_node, fd_set *allset_ptr);
void read_client(Client *c, DLL **match_list_ptr);
Packet **parse_client_buffer(Client *c);
static void log_msg(const char *fmt, ...);

#endif
