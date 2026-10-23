PORT=98377
CFLAGS = -DPORT=$(PORT) -g -Wall
all: client server play

%.o: %.c
	gcc ${CFLAGS} -c $<

server.o: server.c socket.h server.h client_server.h
	gcc ${CFLAGS} -c $<

client.o: client.c socket.h client_server.h
	gcc ${CFLAGS} -c $<

client: client.o ui.o socket.o
	gcc ${CFLAGS} -o client client.o ui.o socket.o -lncurses

server: server.o socket.o doubly_linked_list.o matchmaking.o pokemon.o game.o
	gcc ${CFLAGS} -o $@ $^

play: play.o
	gcc ${CFLAGS} -o $@ $^

clean:
	rm *.o client server play
