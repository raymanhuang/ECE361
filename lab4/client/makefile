CC = gcc
CFLAGS = -Wall -I../shared
LDFLAGS = -L../shared -lshared -pthread

client: client.o
	$(CC) -o client client.o $(LDFLAGS)

client.o: client.c client.h
	$(CC) -c client.c $(CFLAGS)

clean:
	rm -f client client.o
