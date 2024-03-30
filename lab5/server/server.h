// server.h 
#ifndef SERVER_H
#define SERVER_H
#define MAX_USERS 10
#include "common.h"

// Function declarations, type definitions, macros, etc.
typedef struct{
    char username[MAX_NAME];
    char password[MAX_DATA];
} User;

typedef struct{
    User user;
    char ip[INET_ADDRSTRLEN];
    int port;
    char session_id[MAX_DATA];
    int sockfd;
    bool in_session;
} ConnectedUser;

typedef struct ClientNode{
    ConnectedUser client;
    struct ClientNode* next;
} ClientNode;

void add_connected_client(ConnectedUser* new_client);
void print_connected_clients();
bool is_client_already_connected(char* username);
bool check_if_user_exists(const char *username, const char *filename);
ClientNode* find_client_by_username(char* username);
void update_client_session(char* username, char* session_id, bool in_session);
bool is_client_already_in_a_session(char* username);
bool session_exists(char* session_id);

char* get_session_id(char* username);
void broadcast_message_to_all_but_sender(char* session_id, char* text_message, char* sender_username);
void remove_client_from_list(char* username);
char* build_query_list(ClientNode* head);
bool check_credentials(const char* username, const char* password);
bool append_user_credentials(const char *username, const char *password, const char *filename);

#endif // SERVER_H