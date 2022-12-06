CFLAGS = -g -Wall -Wextra
INCLUDE = -lncurses

EXEC_FILE = client server
SOURCES = $(addsuffix .c, $(EXEC_FILE))
OBJECTS= $(SOURCES:.c=.o)

CC = gcc

.PHONY: all clean

all: $(EXEC_FILE)

server: server.c
	$(CC) $(CFLAGS) -pthread $^ -o $(patsubst %.c, %, $<) $(INCLUDE)

%: %.c
	$(CC) $(CFLAGS) $^ -o $(patsubst %.c, %, $<) $(INCLUDE)

clean:
	$(RM) -f $(EXEC_FILE) *.o



hellomake: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)