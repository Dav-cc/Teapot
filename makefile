CC      = gcc
CFLAGS  = -D_GNU_SOURCE -Wall -Wextra -Wpedantic -std=c11 -g
TARGET  = server

SRC = \
	src/main.c\
	src/core/event.c \
	src/core/log.c \
	src/http/server.c \
	src/http/sock.c  \
	src/core/dbuff.c    \
	src/parser/http_parser.c\
	src/http/router.c\
	src/http/http_response.c\


OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all run clean
