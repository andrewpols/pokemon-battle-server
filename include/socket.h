#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <netinet/in.h>    /* Internet domain header, for struct sockaddr_in */

typedef struct {
  int fd;
  struct sockaddr_in peer_socket;
} ClientReturn;


struct sockaddr_in *init_server_addr(int port);
int set_up_server_socket(struct sockaddr_in *self, int num_queue);
ClientReturn *accept_connection(int listenfd);
int connect_to_server(int port, const char *hostname, const char *username);

#endif

