# Variables
CC=gcc
CFLAGS=-Wall
AR=ar
ARFLAGS=rcs
LIBNAME=libshared.a

# This target builds the static library
$(LIBNAME): common.o
	$(AR) $(ARFLAGS) $(LIBNAME) common.o

# This target compiles the common source file into an object file
common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c -o common.o

# The clean target removes the static library and object file
clean:
	rm -f $(LIBNAME) common.o

# Phony targets ensure that make doesn't get confused by files named 'clean' or 'all'
.PHONY: clean all

