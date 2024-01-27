CC=gcc
CFLAGS=-Wall -g
SRCS=deliver.c
OBJS=$(SRCS:.c=.o)
MAIN=deliver

all:    $(MAIN)
	@echo Deliver executable has been compiled

$(MAIN): $(OBJS)
    $(CC) $(CFLAGS) -o $(MAIN) $(OBJS)

.c.o:
    $(CC) $(CFLAGS) -c $<  -o $@

clean:
    rm -f *.o *~ $(MAIN)

depend: $(SRCS)
    makedepend $^
