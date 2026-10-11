# Teapot

A small HTTP server written in C.

Teapot is a project I'm building to understand how HTTP servers work beyond the application layer. Instead of relying on an existing HTTP server library, I'm implementing the core pieces myself, from socket handling and event-driven I/O to HTTP parsing, routing, and response generation.

The project is still under development. My main goal is to build a solid understanding of network programming and systems-level software by working through the problems that come with implementing a server from scratch.

## Features

- TCP socket programming
- Event-driven I/O with `epoll`
- HTTP request parsing and routing
- Dynamic buffers for request and response data
- HTTP response generation
- Static file serving (HTML, images, and other file types)
- Persistent connections with keep-alive

## Build and Run

```sh
git clone https://github.com/Dav-cc/Teapot.git
cd Teapot
make
./server
```

The server listens on port `8080`.

Test it with:

```sh
curl -i http://localhost:8080/
```

