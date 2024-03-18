CC = gcc
CFLAGS = -Wall -I../shared
LDFLAGS = -L../shared -lshared -pthread

server: server.o
	$(CC) -o server server.o $(LDFLAGS)

server.o: server.c server.h
	$(CC) -c server.c $(CFLAGS)

clean:
	rm -f server server.o
