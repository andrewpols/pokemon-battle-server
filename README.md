# pokemon-battle-server

A multiplayer Pokémon battle server written in C. Players connect over TCP, get matched into 1v1 battles, and play in a terminal UI built with ncurses. One single-threaded server handles every client at once using a `select()` event loop.

## Features

- 1v1 matchmaking with a lobby. A player who connects with no opponent waits until a second player joins.
- Turn-based battles. Each player gets a random party of four Pokémon with fixed movesets and stats.
- Six commands: `move`, `switch`, `stats`, `chat`, `quit`, and `rematch`.
- In-match chat between the two players.
- Rematches with a fresh party, or a return to the lobby if either player declines.
- Timestamped server logs for connections, disconnections, and matches found.
- A custom binary packet protocol with buffered reads, so fragmented TCP packets are handled correctly.

## Build and run

You need a Unix-like system with `gcc`, `make`, and the ncurses library.

```
make
./server
```

Then start one client per player, each in its own terminal:

```
./play
```

Each client asks for a hostname (for example `localhost`) and a username of up to 32 bytes.

## How to play

Once two players are matched, these commands are available:

| Command | What it does |
| --- | --- |
| `move [1-4]` | Use one of your active Pokémon's four moves |
| `switch [1-4]` | Swap your active Pokémon (the target must not have fainted) |
| `stats` | Show both parties' HP and your own movesets (the opponent's moves stay hidden) |
| `chat [message]` | Send a message to your opponent |
| `quit` | Forfeit the match |
| `rematch yes` / `rematch no` | Answer the rematch prompt after a match ends |

Each player acts once per turn. When both have acted, the Pokémon with the higher speed attacks first, and the result is sent to both players. A switch uses your action for that turn, so no attack is dealt by that player. A match ends when one player forfeits or all of one player's Pokémon faint.

## Architecture

```
                 listening socket
                        |
                  +-----------+
                  |  server   |   one select() loop
                  +-----------+
                 /      |      \
          client A   client B   client C
          (match)    (match)    (lobby)
```

The server keeps track of state with a generic doubly linked list:

- a global list of every connected client
- a global list of every active match
- a list of the two participating clients inside each match

When a client connects, `matchmake` looks for a lobby match with one waiting player. If it finds one, the client joins and the battle starts. If not, a new match is created and the client waits.

### Concurrency

The server is single-threaded and uses one `select()` call per loop iteration. It keeps two `fd_set`s: `allset` holds every active file descriptor, and `rset` is a working copy passed to `select()` each time, since `select()` modifies it in place.

On each iteration the server checks the listening socket for new connections, then checks each client's descriptor with `FD_ISSET` and reads only from the ones that are ready. No single client can block the others. A descriptor is removed from `allset` with `FD_CLR` before it is closed, so a closed descriptor is never passed to `select()`. The loop saves the next list node before handling each client, so removing a client mid-iteration does not break the traversal.

Because everything runs in one thread, there are no race conditions and no locking.

## Communication protocol

All communication uses TCP.

**Handshake.** The client first sends its username as a raw 32-byte array. The server reads it with `recv` and `MSG_WAITALL`. This is the only message that does not use the packet format below.

**Packets.** Everything after the handshake, in both directions, uses this format:

```
+----------+------------------------+---------------------------+
| cmd_type | payload_length         | payload                   |
| 1 byte   | 4 bytes, network order | null-terminated string    |
+----------+------------------------+---------------------------+
```

The receiver reads the 5-byte header, converts `payload_length` with `ntohl`, and waits until the full payload has arrived before processing the message. Packets are stored in a dynamically allocated struct with a flexible array member for the payload.

| Command | Value | Direction | Payload |
| --- | --- | --- | --- |
| `CMD_MOVE` | 0 | client to server | `"1"` to `"4"` |
| `CMD_SWITCH` | 1 | client to server | `"1"` to `"4"` |
| `CMD_STATS` | 2 | client to server | empty |
| `CMD_CHAT` | 3 | both | chat text from clients, all game messages from the server |
| `CMD_QUIT` | 4 | client to server | empty |
| `CMD_REMATCH` | 5 | client to server | `"yes"` or `"no"` |

On the client side, every incoming packet is a `CMD_CHAT`, and its payload is printed straight to the ncurses display.

## Error handling

- **Client disconnects mid-match.** `read` returns 0, the server closes the descriptor, removes it from `allset`, cleans up the match, tells the opponent, and returns them to the lobby.
- **Server shuts down mid-match.** The client sees the closed connection, shuts down ncurses, prints a message, and exits cleanly.
- **Bad hostname.** The client checks the `getaddrinfo` result, prints an error to stderr, and exits.
- **Invalid commands.** Out-of-range move or switch indexes, switching to a fainted Pokémon, and commands sent in the wrong state are rejected with an explanatory message. No game state changes.
- **Fragmented packets.** Both sides buffer reads across multiple `read` calls and only process a packet once the header and the full payload are present.

## Debugging

Memory errors and use-after-free bugs were tracked down with AddressSanitizer and Valgrind.
