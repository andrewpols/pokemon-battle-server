#ifndef _CLIENT_SERVER_H_
#define _CLIENT_SERVER_H_

#define MAX_CLIENT_MSG_SIZE 128

#define CLIENT_BUF_SIZE 256
#define SERVER_BUF_SIZE 2048

typedef enum {
  CMD_MOVE = 0,
  CMD_SWITCH = 1,
  CMD_STATS = 2,
  CMD_CHAT = 3,
  CMD_QUIT = 4,
  CMD_REMATCH = 5
} CommandType;

typedef struct {
  uint8_t cmd_type;
  uint32_t payload_length;
  char payload[];
} Packet;

typedef struct {
  char *name;
  CommandType type;
} CommandMap;

static inline Packet *make_packet(CommandType type, const char *payload) {
  size_t len = strlen(payload);
  Packet *pkt = malloc(sizeof(Packet) + len + 1);
  pkt->cmd_type = type;
  pkt->payload_length = (uint32_t)len;
  strncpy(pkt->payload, payload, len);
  pkt->payload[len] = '\0';
  return pkt;
}


#endif
