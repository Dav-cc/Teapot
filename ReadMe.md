# Teapot

A lightweight, event-driven HTTP server written in C for Linux. Built as an educational project to explore high-performance network programming with non-blocking I/O and epoll.

## Features

- Single-threaded event loop built on Linux `epoll`
- Non-blocking sockets with edge-triggered events
- Incremental HTTP/1.1 request parser
- Per-connection read and write buffers backed by a ring buffer
- Keep-alive connection handling
- Simple leveled logging module

## Project Structure

```
src/
├── main.c               # Entry point
├── core/
│   ├── event.c          # Epoll-based event loop
│   ├── log.c            # Logging utilities
│   └── rb.c             # Ring buffer
├── http/
│   ├── server.c         # Server setup and connection handling
│   └── sock.c           # Socket helpers
└── parser/
    └── http_parser.c    # HTTP request parser
```

## Requirements

- Linux (epoll required)
- GCC
- Make

## Building

```sh
make
```

## Running

```sh
make run
```

The server listens on port 8080:

```sh
curl http://localhost:8080/
```

## License

Distributed under the MIT License. See [LICENSE](LICENSE) for details.
