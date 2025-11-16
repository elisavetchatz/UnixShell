CC = gcc
CFLAGS = -Wall -Wextra -g

all: tinyshell

tinyshell: tinyShell.c
	$(CC) $(CFLAGS) tinyShell.c -o tinyShell

clean:
	rm -f tinyshell

.PHONY: all clean
