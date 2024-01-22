# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g

# Executable names
DELIVER = deliver
SERVER = server

# Default target
all: $(DELIVER) $(SERVER)

# Compile deliver executable
$(DELIVER): deliver.o
	$(CC) $(CFLAGS) -o $(DELIVER) deliver.o

# Compile server executable
$(SERVER): server.o
	$(CC) $(CFLAGS) -o $(SERVER) server.o

# Compile deliver.o
deliver.o: deliver.c
	$(CC) $(CFLAGS) -c deliver.c

# Compile server.o
server.o: server.c
	$(CC) $(CFLAGS) -c server.c

# Clean target
clean:
	rm -f $(DELIVER) $(SERVER) *.o
