// server.h or common.h
#ifndef CLIENT_H
#define CLIENT_H

// Function declarations, type definitions, macros, etc.
#define CLIENT_PROMPT "client> "
#define MAX_ARGS 5

typedef enum {
    INVALID_COMMAND = -1,
    LOG_IN,
    LOGOUT,
    JOIN_SESSION,
    LEAVE_SESSION,
    CREATE_SESSION,
    LIST,
    QUIT,
    TEXT
} Command;

void displayPrompt();
Command handleCommand(char** args);

#endif // CLIENT_H