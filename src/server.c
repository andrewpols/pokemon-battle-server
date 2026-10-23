#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#include "socket.h"
#include "server.h"
#include "client_server.h"
#include "matchmaking.h"
#include "log.h"

#define MAX_USERNAME_SIZE 32

static volatile int running = 1;

static void handle_sigint(int sig) {
  (void)sig;
  running = 0;
}

int main() {
  signal(SIGINT, handle_sigint);

  struct sockaddr_in *self = init_server_addr(PORT);
  int listen_fd = set_up_server_socket(self, 5);
  free(self);

  DLL *match_list = NULL;
  DLL *client_list = NULL;

  fd_set allset;
  fd_set rset;
  int max_fd = listen_fd;

  FD_ZERO(&allset);
  FD_SET(listen_fd, &allset);

  while (running) {
    rset = allset;

    if (select(max_fd + 1, &rset, NULL, NULL, NULL) == -1) {
      if (!running)
        break;
      perror("select");
      exit(1);
    }

    if (FD_ISSET(listen_fd, &rset)) {
      ClientReturn *c = accept_connection(listen_fd);
      if (add_client(c, &match_list, &client_list, &allset, &max_fd) == -1) {
        fprintf(stderr, "Error establishing connection to client.\n\n");
        continue;
      }
    }

    DLL *curr = client_list;
    while (curr != NULL) {
      Client *client = (Client *)curr->data;
      DLL *next = curr->next;
      if (FD_ISSET(client->fd, &rset)) {
        read_client(client, &match_list);
        if (client->status == DISCONNECTING) {
          struct in_addr ipaddr = client->ipaddr;
          uint16_t port = client->port;
          char username[MAX_USERNAME_SIZE];
          strncpy(username, client->username, MAX_USERNAME_SIZE);

          FD_CLR(client->fd, &allset);
          close(client->fd);
          remove_user_from_match(client, &match_list);
          free(client->username);
          free(client);

          remove_node(&client_list, curr);

          log_msg("Disconnected: %s:%d (%s)", inet_ntoa(ipaddr), port, username);
        }
      }
      curr = next;
    }
  }

  DLL *mcurr = match_list;
  while (mcurr != NULL) {
    Match *match = (Match *)mcurr->data;
    DLL *mnext = mcurr->next;
    free(match);
    free(mcurr);
    mcurr = mnext;
  }

  close(listen_fd);
  return 0;
}

int add_client(ClientReturn *c, DLL **match_list_ptr, DLL **client_list_ptr, fd_set *allset_ptr, int *max_fd) {
  if (c == NULL)
    return -1;

  Client *new_client = malloc(sizeof(Client));
  memset(new_client, 0, sizeof(Client));

  new_client->fd = c->fd;
  new_client->status = IN_LOBBY;
  new_client->ipaddr = c->peer_socket.sin_addr;
  new_client->port = ntohs(c->peer_socket.sin_port);
  new_client->buf_len = 0;
  new_client->chosen_move = 0;

  assign_random_party(&new_client->party);

  new_client->username = malloc(MAX_USERNAME_SIZE);
  if (recv(c->fd, new_client->username, MAX_USERNAME_SIZE, MSG_WAITALL) < 0) {
    perror("Error reading client username");
    return -1;
  }

  log_msg("New connection from %s:%d (%s)", inet_ntoa(new_client->ipaddr), new_client->port, new_client->username);

  add_node(client_list_ptr, new_client);
  matchmake(match_list_ptr, new_client);
  FD_SET(new_client->fd, allset_ptr);
  if (new_client->fd > *max_fd)
    *max_fd = new_client->fd;

  free(c);
  return 0;
}

void read_client(Client *client, DLL **match_list_ptr) {
  int nbytes = read(client->fd, &client->buf[client->buf_len], CLIENT_BUF_SIZE - client->buf_len);

  if (nbytes < 0) {
    perror("read");
    return;
  }

  if (nbytes == 0) {
    client->status = DISCONNECTING;
    return;
  }

  client->buf_len += nbytes;

  Packet **messages = parse_client_buffer(client);
  if (messages == NULL)
    return;

  Packet **curr = messages;
  while (*curr != NULL) {
    handle_command(client, *curr, match_list_ptr);
    free(*curr);
    curr++;
  }
  free(messages);
}

Packet **parse_client_buffer(Client *c) {
  int max_msgs = (CLIENT_BUF_SIZE / 3) + 1;
  Packet **messages = malloc(max_msgs * sizeof(Packet *));
  int n_msgs = 0;
  int header_len = sizeof(uint8_t) + sizeof(uint32_t);

  while (c->buf_len >= header_len) {
    uint32_t payload_length;
    memcpy(&payload_length, &c->buf[1], sizeof(uint32_t));
    payload_length = ntohl(payload_length);

    if (c->buf_len < header_len + payload_length)
      break;

    CommandType type = (CommandType)c->buf[0];

    Packet *message = malloc(sizeof(Packet) + payload_length + 1);
    message->cmd_type = type;
    message->payload_length = payload_length;
    strncpy(message->payload, &c->buf[header_len], payload_length);
    message->payload[payload_length] = '\0';

    c->buf_len -= header_len + payload_length;
    memmove(c->buf, &c->buf[header_len + payload_length], c->buf_len);

    messages[n_msgs++] = message;
  }

  messages[n_msgs] = NULL;
  if (messages[0] == NULL) {
    free(messages);
    return NULL;
  }
  return messages;
}
