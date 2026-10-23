#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <errno.h>
#include "socket.h"
#include "client_server.h"
#include "ui.h"

#ifndef PORT
#define PORT 30000
#endif

#define MAX_USERNAME_SIZE 32
#define NUM_CMD 6

typedef struct {
  int fd;
  char buf[SERVER_BUF_SIZE];
  int buf_len;
} Server;

static CommandMap commands[] = {
  { "move", CMD_MOVE },
  { "switch", CMD_SWITCH },
  { "stats", CMD_STATS },
  { "chat", CMD_CHAT },
  { "q", CMD_QUIT },
  { "rematch", CMD_REMATCH }
};

Packet *build_packet(char *user_input);
void send_packet(int fd, Packet *pkt);
Packet **read_server(Server *s, int *disconnected);
Packet **parse_server_buffer(Server *s);

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: client hostname username\n");
    exit(1);
  }

  char username[MAX_USERNAME_SIZE] = { '\0' };
  strncpy(username, argv[2], MAX_USERNAME_SIZE);
  username[MAX_USERNAME_SIZE - 1] = '\0';

  if (strlen(username) == 0) {
    fprintf(stderr, "Usernames must be at least one character.\n");
    exit(1);
  }

  int soc_fd = connect_to_server(PORT, argv[1], username);
  Server server = { .fd = soc_fd, .buf_len = 0 };

  ui_init();
  ui_set_status("Waiting for opponent... | move <1-4> | switch <1-4> | stats | chat <msg> | q");

  int max_fd = soc_fd;
  char user_buf[MAX_CLIENT_MSG_SIZE];
  int buf_pos = 0;
  memset(user_buf, 0, MAX_CLIENT_MSG_SIZE);

  wtimeout(input_win, 0);

  while (1) {
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(soc_fd, &rset);
    struct timeval tv = { .tv_sec = 0, .tv_usec = 50000 };

    if (select(max_fd + 1, &rset, NULL, NULL, &tv) < 0) {
      if (errno == EINTR)
        continue; // terminal resize, just retry
      ui_cleanup();
      perror("select");
      break;
    }

    if (FD_ISSET(soc_fd, &rset)) {
      int disconnected = 0;
      Packet **messages = read_server(&server, &disconnected);
      if (messages != NULL) {
        for (int i = 0; messages[i] != NULL; i++) {
          ui_print(messages[i]->payload);
          free(messages[i]);
        }
        free(messages);
      }
      if (disconnected) {
        sleep(5); // let user read the final message in ncurses
        ui_cleanup();
        printf("You have been disconnected.\n");
        break;
      }
    }

    int ch = wgetch(input_win);
    if (ch == ERR)
      continue;
    if (ch == 27) { // escape sequence start
      while (wgetch(input_win) != ERR)
        ; // drain the rest
      continue;
    }
    if (ch > 255 && ch != KEY_BACKSPACE)
      continue;

    if (ch == '\n' || ch == '\r') {
      if (buf_pos > 0) {
        user_buf[buf_pos] = '\0';
        Packet *pkt = build_packet(user_buf);
        if (pkt != NULL) {
          send_packet(soc_fd, pkt);
        }
        buf_pos = 0;
        memset(user_buf, 0, MAX_CLIENT_MSG_SIZE);
        werase(input_win);
        wprintw(input_win, "> ");
        wrefresh(input_win);
      }
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      if (buf_pos > 0) {
        buf_pos--;
        int y, x;
        getyx(input_win, y, x);
        mvwdelch(input_win, y, x - 1);
        wrefresh(input_win);
      }
    } else if (ch >= 32 && ch < 127 && buf_pos < MAX_CLIENT_MSG_SIZE - 1) {
      user_buf[buf_pos++] = (char)ch;
      waddch(input_win, ch);
      wrefresh(input_win);
    }
  }

  ui_cleanup();
  close(soc_fd);
  return 0;
}

Packet *build_packet(char *user_input) {
  Packet *pkt = malloc(sizeof(Packet) + MAX_CLIENT_MSG_SIZE);
  char buf[MAX_CLIENT_MSG_SIZE];

  if (sscanf(user_input, "%15s", buf) == 0) {
    ui_print("Invalid command. Usage: [CMD_TYPE] [ARG]");
    free(pkt);
    return NULL;
  }

  int cmd_type = -1;
  for (int i = 0; i < NUM_CMD; i++) {
    if (strcmp(buf, commands[i].name) == 0) {
      cmd_type = (int)commands[i].type;
    }
  }

  if (cmd_type == -1) {
    ui_print("Unknown command. Valid commands: move | switch | stats | chat | q");
    free(pkt);
    return NULL;
  }

  pkt->cmd_type = cmd_type;

  if (sscanf(user_input, "%*s %63[^\n]", buf) != 1) {
    pkt->payload_length = 0;
    pkt->payload[0] = '\0';
    return pkt;
  }

  buf[MAX_CLIENT_MSG_SIZE - 1] = '\0';
  pkt->payload_length = (uint32_t)strlen(buf);
  strncpy(pkt->payload, buf, MAX_CLIENT_MSG_SIZE);
  pkt->payload[MAX_CLIENT_MSG_SIZE - 1] = '\0';

  return pkt;
}

void send_packet(int fd, Packet *pkt) {
  if (pkt == NULL)
    return;
  uint32_t net_len = htonl(pkt->payload_length);
  write(fd, &pkt->cmd_type, sizeof(uint8_t));
  write(fd, &net_len, sizeof(uint32_t));
  write(fd, pkt->payload, pkt->payload_length);
  free(pkt);
}

Packet **read_server(Server *s, int *disconnected) {
  *disconnected = 0;
  int nbytes = read(s->fd, &s->buf[s->buf_len], SERVER_BUF_SIZE - s->buf_len);
  if (nbytes < 0) {
    perror("read");
    *disconnected = 1;
    return NULL;
  }
  if (nbytes == 0) {
    *disconnected = 1;
    return NULL;
  }
  s->buf_len += nbytes;
  return parse_server_buffer(s);
}

Packet **parse_server_buffer(Server *s) {
  int max_msgs = (SERVER_BUF_SIZE / 3) + 1;
  Packet **messages = malloc(max_msgs * sizeof(Packet *));
  int n_msgs = 0;
  int header_len = sizeof(uint8_t) + sizeof(uint32_t);

  while (s->buf_len >= header_len) {
    uint32_t payload_length;
    memcpy(&payload_length, &s->buf[1], sizeof(uint32_t));
    payload_length = ntohl(payload_length);

    if (s->buf_len < header_len + (int)payload_length)
      break;

    CommandType type = (CommandType)s->buf[0];
    Packet *message = malloc(sizeof(Packet) + payload_length + 1);
    message->cmd_type = type;
    message->payload_length = payload_length;
    strncpy(message->payload, &s->buf[header_len], payload_length);
    message->payload[payload_length] = '\0';

    s->buf_len -= header_len + payload_length;
    memmove(s->buf, &s->buf[header_len + payload_length], s->buf_len);

    messages[n_msgs++] = message;
  }

  messages[n_msgs] = NULL;
  if (messages[0] == NULL) {
    free(messages);
    return NULL;
  }
  return messages;
}
