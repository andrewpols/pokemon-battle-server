#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> /* inet_ntoa */
#include <netdb.h>     /* gethostname */
#include <sys/socket.h>

#include "socket.h"

#define MAX_USERNAME_SIZE 32

/*
 * Initialize a server address (for listening) associated with the given port.
 */

struct sockaddr_in *init_server_addr(int port) {
  struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in));

  // Allow sockets across machines via the internet.
  addr->sin_family = AF_INET;

  // The port the process will listen on, and where the socket will handle connection requests.
  addr->sin_port = htons(port);

  // Listen on all addresses of the machine
  addr->sin_addr.s_addr = INADDR_ANY;

  // Clear for padding.
  memset(&(addr->sin_zero), 0, 8);

  return addr;
}

/*
 * Create and set up a socket for a server (identified by sockaddr_in) to listen on.
 */
int set_up_server_socket(struct sockaddr_in *self, int num_queue) {
  // file descriptor associated with the socket set up for listening
  int soc = socket(AF_INET, SOCK_STREAM, 0);
  if (soc < 0) {
    perror("socket");
    exit(1);
  }

  // Make sure we can reuse the port immediately after the
  // server terminates. Avoids the "address in use" error
  int on = 1;
  int status = setsockopt(soc, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));
  if (status < 0) {
    perror("setsockopt");
    exit(1);
  }

  // Associate the server process with the address and port, bound to our socket
  if (bind(soc, (struct sockaddr *)self, sizeof(*self)) < 0) {
    // bind failed; possibly bc port is in use
    perror("bind");
    exit(1);
  }

  // Set up the socket for listening, and initialize a queue to hold partial connections
  if (listen(soc, num_queue) < 0) {
    perror("listen");
    exit(1);
  }

  return soc;
}

/*
 * Wait for and accept a new connection.
 * Return the file descriptor of the newly created server socket to communicate with the client.
 * Return -1 if the accept call failed (since this is recoverable).
 */
ClientReturn *accept_connection(int listenfd) {
  struct sockaddr_in peer;
  unsigned int peer_len = sizeof(peer);
  peer.sin_family = AF_INET;

  int client_fd = accept(listenfd, (struct sockaddr *)&peer, &peer_len);

  if (client_fd < 0) {
    perror("accept");
    return NULL;
  }

  ClientReturn *c = malloc(sizeof(ClientReturn));
  c->fd = client_fd;
  c->peer_socket = peer;

  return c;
}

/******************************************************************************
 * Client-specific functions
 *****************************************************************************/
/*
 * Create a socket and connect to the server indicated by the port and hostname
 * Return file descriptor of the created client socket.
 */
int connect_to_server(int port, const char *hostname, const char *username) {
  int client_soc = socket(AF_INET, SOCK_STREAM, 0);
  if (client_soc < 0) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_in server_addr;

  // Allow sockets across machines
  server_addr.sin_family = AF_INET;
  // The port the server will be listening on.
  server_addr.sin_port = htons(port);
  // Clear for padding
  memset(&(server_addr.sin_zero), 0, 8);

  // Lookup host address info and store in result
  struct addrinfo *result;
  if (getaddrinfo(hostname, NULL, NULL, &result) != 0) {
    fprintf(stderr, "Unknown host: %s\n", hostname);
    exit(1);
  }

  if (result == NULL) {
    fprintf(stderr, "unkown host %s\n", hostname);
    exit(1);
  }

  server_addr.sin_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr;
  freeaddrinfo(result);

  // Request connection to server.
  printf("Requesting connection to %s...\n", hostname);
  if (connect(client_soc, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    perror("connect");
    exit(1);
  }

  // connection accepted; send username to server
  write(client_soc, username, MAX_USERNAME_SIZE);

  printf("Succesfully connected to server!\n\n");

  return client_soc;
}
