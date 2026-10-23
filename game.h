#ifndef GAME_H
#define GAME_H

#include "client_server.h"
#include "matchmaking.h"

void handle_command(Client *client, Packet *pkt, DLL **match_list_ptr);
void send_match_start(Match *match);
void send_to_client(Client *client, const char *msg);


#endif
