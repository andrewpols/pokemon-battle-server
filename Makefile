CC ?= cc
PORT ?= 30000
BUILD_DIR ?= build
CPPFLAGS += -Iinclude -DPORT=$(PORT)
CFLAGS ?= -g -Wall
LDFLAGS ?=
LDLIBS ?=

CLIENT_OBJECTS = $(BUILD_DIR)/client.o $(BUILD_DIR)/ui.o $(BUILD_DIR)/socket.o
SERVER_OBJECTS = $(BUILD_DIR)/server.o $(BUILD_DIR)/socket.o \
	$(BUILD_DIR)/doubly_linked_list.o $(BUILD_DIR)/matchmaking.o \
	$(BUILD_DIR)/pokemon.o $(BUILD_DIR)/game.o

.PHONY: all clean client server play

all: client server play

client: $(BUILD_DIR)/client

server: $(BUILD_DIR)/server

play: $(BUILD_DIR)/play

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/client: $(CLIENT_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lncurses

$(BUILD_DIR)/server: $(SERVER_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/play: $(BUILD_DIR)/play.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -rf $(BUILD_DIR)
