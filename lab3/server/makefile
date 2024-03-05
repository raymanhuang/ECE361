CC=gcc
CFLAGS=-Wall -g
SRCS=server.c
OBJS=$(SRCS:.c=.o)
MAIN=server

all:	$(MAIN)
	@echo Server executable has been compiled

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $<  -o $@

clean:
	rm -f *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $^
