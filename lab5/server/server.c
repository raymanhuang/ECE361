#include "common.h"
#include "server.h"
#define MAX_USERS 10
#define MAX_SESSIONS 10

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

ClientNode *head = NULL;

User users[MAX_USERS] = {
    {"suu1", "dunno"},
    {"suu2", "dunno"},
    {"suu3", "dunno"},
    {"suu4", "dunno"},
    {"suu5", "dunno"},
    {"suu6", "dunno"},
    {"suu7", "dunno"},
    {"suu8", "dunno"},
    {"suu9", "dunno"},
    {"suu10", "dunno"}
};

void* client_connection(void *arg){
    int client_sockfd = *(int *)arg;
    free(arg);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Get the address of the connected client
    if (getpeername(client_sockfd, (struct sockaddr *)&client_addr, &client_addr_len) < 0) {
        perror("getpeername failed");
        close(client_sockfd);
        return NULL;
    }

    // Convert the client's IP address to a human-readable string
    char client_ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip)) == NULL) {
        perror("inet_ntop failed");
        close(client_sockfd);
        return NULL;
    }
    int port = ntohs(client_addr.sin_port);

    while(1){
        message msg;
        receive_message(client_sockfd, &msg);
        printf("RECEIVED MSG FROM CLIENT %s\n", msg.source);
        if(msg.type == LOGIN){
            printf("received login msg\n");
            printf("from: %s with client_sockfd: %d\n", msg.source, client_sockfd);
            printf("password: %s\n", msg.data);
            if(check_if_user_exists(msg.source, msg.data) == false){
                message login_failed;
                login_failed.type = LO_NAK;
                login_failed.size = strlen("Login failed, User not found!");
                strcpy((char*)login_failed.source, "server");
                strcpy((char*)login_failed.data, "Login failed, User not found!");
                send_message(client_sockfd, &login_failed);
            }
            else if(is_client_already_connected(msg.source) == true){
                message login_failed;
                login_failed.type = LO_NAK;
                login_failed.size = strlen("Login failed, User already connected!");
                strcpy((char*)login_failed.source, "server");
                strcpy((char*)login_failed.data, "Login failed, User already connected!");
                send_message(client_sockfd, &login_failed);
            }
            else {
                ConnectedUser new_client;
                new_client.in_session = false;
                strcpy(new_client.ip, client_ip);
                new_client.port = port;
                printf("%s, %d\n", new_client.ip, new_client.port);
                strcpy((char*)new_client.user.username, msg.source);
                //printf("made it here 1\n");
                strcpy((char*)new_client.user.password, msg.data);
                //printf("made it here 2\n");
                new_client.sockfd = client_sockfd;
                add_connected_client(&new_client);
                //printf("Added %s\n", head->client.user.username);
                print_connected_clients();

                message login_success;
                login_success.type = LO_ACK;
                login_success.size = strlen("Login successful!");
                strcpy((char*)login_success.source, "server");
                strcpy((char*)login_success.data, "Login successful!");
                send_message(client_sockfd, &login_success);
            }
        } else if (msg.type == JOIN){
            if(is_client_already_in_a_session(msg.source) == true){
                message join_session_failed;
                join_session_failed.type = JN_NAK;
                join_session_failed.size = strlen("You are already in a session, Leave first!");
                strcpy((char*)join_session_failed.source, "server");
                strcpy((char*)join_session_failed.data, "You are already in a session, Leave first!");
                send_message(client_sockfd, &join_session_failed);
            } else {
                if(session_exists(msg.data) == true){
                    update_client_session(msg.source, msg.data, true);
                    print_connected_clients();
                    message join_session_success;
                    join_session_success.type = JN_ACK;
                    join_session_success.size = strlen("Session Joined!");
                    strcpy((char*)join_session_success.source, "server");
                    strcpy((char*)join_session_success.data, "Session Joined");
                    send_message(client_sockfd, &join_session_success);
                } else {
                    message join_session_failed;
                    join_session_failed.type = JN_NAK;
                    join_session_failed.size = strlen("Session not found!");
                    strcpy((char*)join_session_failed.source, "server");
                    strcpy((char*)join_session_failed.data, "Session not found!");
                    send_message(client_sockfd, &join_session_failed);
                }
            }
            
        } else if (msg.type == NEW_SESS){
            if(is_client_already_in_a_session(msg.source) == true){
                message new_session_failed;
                new_session_failed.type = NS_NAK;
                new_session_failed.size = strlen("You are already in a session, Leave first!");
                strcpy((char*)new_session_failed.source, "server");
                strcpy((char*)new_session_failed.data, "You are already in a session, Leave first!");
                send_message(client_sockfd, &new_session_failed);
            }
            else{
                message new_session_success;
                new_session_success.type = NS_ACK;
                char session_message[256];
                sprintf(session_message, "Joined session %s", msg.data);
                new_session_success.size = strlen(session_message);
                strcpy((char*)new_session_success.source, "server");
                strcpy((char *)new_session_success.data, session_message);
                send_message(client_sockfd, &new_session_success);
                update_client_session(msg.source, msg.data, true);
                print_connected_clients();
            }
        } else if (msg.type == MESSAGE){
            if(is_client_already_in_a_session(msg.source) == false){
                message message_failed;
                message_failed.type = MSG_NAK;
                message_failed.size = strlen("Join a session first!");
                strcpy((char*)message_failed.source, "server");
                strcpy((char*)message_failed.data, "Join a session first!");
                send_message(client_sockfd, &message_failed);
            }
            else{
                // get session ID
                // send message to all clients with matching session ID
                broadcast_message_to_all_but_sender(get_session_id(msg.source), msg.data, msg.source);
            }
        } else if (msg.type == PRIVATE_MSG) {
            // getting the recipients name
            char recipient[MAX_NAME];
            char* messageContent = strchr((char*)msg.data, ':');
            
            if (messageContent) {
                *messageContent = '\0'; // replacing : with \0 to end the recipients username string
                strcpy(recipient, (char*)msg.data);
                messageContent++; 

                // finding the client and forwarding the msg to that client
                ClientNode* target = find_client_by_username(recipient);
                if (target) {
                    message forwardMsg;
                    forwardMsg.type = PRIVATE_MSG;
                    snprintf((char*)forwardMsg.data, MAX_DATA, "%s:%s", msg.source, messageContent);
                    forwardMsg.size = strlen((char*)forwardMsg.data);
                    strcpy((char*)forwardMsg.source, msg.source);
                    send_message(target->client.sockfd, &forwardMsg);
                // if the recipient was not found, alert the sender
                } else {
                    message feedbackMsg;
                    feedbackMsg.type = MSG_NAK; 
                    snprintf((char*)feedbackMsg.data, MAX_DATA, "'%s' not found or not connected", recipient);
                    feedbackMsg.size = strlen((char*)feedbackMsg.data);
                    strcpy((char*)feedbackMsg.source, "server");
                    send_message(client_sockfd, &feedbackMsg);
                }
            // invalid formatting, alert the sender
            } else {
                message feedbackMsg;
                feedbackMsg.type = MSG_NAK; 
                strcpy((char*)feedbackMsg.data, "Invalid format, your private message was not delivered");
                feedbackMsg.size = strlen((char*)feedbackMsg.data);
                strcpy((char*)feedbackMsg.source, "server");
                send_message(client_sockfd, &feedbackMsg); 
            }
        } else if (msg.type == LEAVE_SESS){
            printf("leave request from %s\n", msg.source);
            update_client_session(msg.source, "", false);
            print_connected_clients();
        } else if (msg.type == EXIT){
            // remove client from list
            message logout_ack;
            logout_ack.type = LGO_ACK;
            logout_ack.size = strlen("logout_ack");
            strcpy((char*)logout_ack.source, "server");
            strcpy((char*)logout_ack.data, "logout_ack");
            send_message(client_sockfd, &logout_ack);
            remove_client_from_list(msg.source);
            print_connected_clients();
            printf("closing: %s, client_sockfd: %d\n", msg.source, client_sockfd);
            close(client_sockfd);
            break;
        }
        else if (msg.type == QUERY){
            message query_ack;
            query_ack.type = QU_ACK;
            strcpy((char*)query_ack.source, "server");
            char* list_to_send = build_query_list(head);
            printf("list:\n %s", list_to_send);
            query_ack.size = strlen(list_to_send);
            strcpy((char*)query_ack.data, list_to_send);
            free(list_to_send);
            send_message(client_sockfd, &query_ack);
        }
    }
    printf("client terminated\n");
}

int main(int argc, char* argv[]){
    if (argc != 2) {
        printf("Usage: %s <UDP listen port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        printf("Invalid Port Number!\n");
        return 1;
    }

    int sockfd, *client_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t thread;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Checks the return value of bind
    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("Try again with a different port number!\n");
        return 1;
    }
        // Listen for incoming connections
    if (listen(sockfd, MAX_USERS) < 0) {
        perror("Listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on port %d\n", port);

    while(1){
        client_sockfd = malloc(sizeof(int));
        printf("waiting for client to connect...\n");
        *client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (*client_sockfd < 0) {
            perror("Accept failed");
            free(client_sockfd);
            continue;
        }
        if (pthread_create(&thread, NULL, client_connection, (void *)client_sockfd) != 0) {
            perror("Failed to create thread");
            close(*client_sockfd);
            free(client_sockfd);
            continue;
        }
        pthread_detach(thread);
    }

    pthread_mutex_destroy(&mutex);
    close(sockfd);
    return 0;
}

void add_connected_client(ConnectedUser* new_client){
    printf("entered\n");
    //pthread_mutex_lock(&mutex);
    ClientNode *new_node = (ClientNode *)malloc(sizeof(ClientNode));
    printf("exiting 1\n");
    if (new_node == NULL) {
        perror("Failed to allocate memory for new client node");
        // pthread_mutex_unlock(&mutex); // Unlock the mutex before returning
        return;
    }
    // Copy the client data to the new node
    new_node->client = *new_client;
    new_node->next = head; // Insert the new node at the beginning of the list
    head = new_node; // Update the head of the list

    //pthread_mutex_unlock(&mutex); // Unlock the mutex
    printf("finished adding %s\n", head->client.user.username);
    return;
}

void print_connected_clients(){
    //pthread_mutex_lock(&mutex);
    ClientNode* current;
    if (head == NULL){
        printf("No connected clients\n");
        return;
    }
    // else{
    //     printf("list isnt empty, %s\n", head->client.user.username);
    // }
    printf("Connected Clients:\n");
    current = head;
    // printf("current: %s\n", current->client.user.username);
    int i = 1;
    while(current != NULL){
        // printf("entered while loop\n");
        printf("%d) %s \n", i, current->client.user.username);
        if(current->client.in_session == true){
            printf("connected to %s\n", current->client.session_id);
        } else {
            printf("not in any sessions\n");
        }
        i++;
        current = current->next;
    }
    // printf("exited while loop\n");
    //pthread_mutex_unlock(&mutex);
    return;
}

bool is_client_already_connected(char* username){
    ClientNode* current;
    if (head == NULL){
        return false;
    }
    current = head;
    while(current != NULL){
        if(strcmp(current->client.user.username, username) == 0){
            return true;
        }
        current = current->next;
    }
    return false;
}

bool check_if_user_exists(char* username, char* password){
    for(int i = 0; i < MAX_USERS; i++){
        if(strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0){
            return true;
        }
    }
    return false;
}

ClientNode* find_client_by_username(char* username) {
    ClientNode* current = head;
    while (current != NULL) {
        if (strcmp(current->client.user.username, username) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

bool is_client_already_in_a_session(char* username){
    ClientNode* current;
    if (head == NULL){
        return false;
    }
    current = head;
    while(current != NULL){
        if(strcmp(current->client.user.username, username) == 0){
            if(current->client.in_session == false){
                return false;
            } else{
                return true;
            }
        }
        current = current->next;
    }
    return false;
}

void update_client_session(char* username, char* session_id, bool in_session){
    //pthread_mutex_lock(&mutex);
    ClientNode* current;
    if (head == NULL){
        pthread_mutex_unlock(&mutex); 
        return;
    }
    current = head;
    while(current != NULL){

        if(strcmp(current->client.user.username, username) == 0){
           printf("found node to update\n"); 
           current->client.in_session = in_session;
           strcpy(current->client.session_id, session_id);
           pthread_mutex_unlock(&mutex); 
           return;
        }
        current = current->next;
    }
    printf("node not found\n"); 
    //pthread_mutex_unlock(&mutex); 
    return;
}

bool session_exists(char* session_id){
    ClientNode* current;
    if(head == NULL){
        return false;
    }
    current = head;
    while(current != NULL){
        if(strcmp(current->client.session_id, session_id) == 0){
            printf("session exists\n");
            return true;
        }
        current = current->next;
    }
    printf("session not found\n");
    return false;
}

char* get_session_id(char* username){
    ClientNode* current;
    if(head == NULL){
        return "";
    }
    current = head;
    while(current != NULL){
        if(strcmp(current->client.user.username, username) == 0 && current->client.in_session){
            return current->client.session_id;
        }
        current = current->next;
    }
    printf("session_id not found\n");
    return "";
}

void broadcast_message_to_all_but_sender(char* session_id, char* text_message, char* sender_username){
    ClientNode* current;
    if(head == NULL){
        return;
    }
    current = head;
    printf("Broadcasting: %s\n", text_message);
    while(current != NULL){
        if(strcmp(current->client.session_id, session_id) == 0 && current->client.in_session && strcmp(current->client.user.username, sender_username) != 0){
            // send
           message text;
           text.type = MESSAGE;
           text.size = strlen(text_message);
           strcpy((char*)text.source, sender_username); 
           strcpy((char*)text.data, text_message);
           printf("sending message to file descriptor: %d, of %s\n", current->client.sockfd, current->client.user.username);
           send_message(current->client.sockfd, &text); 
           
        }
        current = current->next;
    }
    return;
}

void remove_client_from_list(char* userid){
    //pthread_mutex_lock(&mutex);
    ClientNode* current = head;
    ClientNode *prev = NULL;

    while (current != NULL) {
        if (strcmp(current->client.user.username,userid) == 0) {
            // Found the node to remove
            printf("found user to remove: %s\n", current->client.user.username);
            if (prev == NULL) {
                // The node to remove is the head of the list
                head = current->next;
            } else {
                // The node to remove is not the head
                prev->next = current->next;
            }
            free(current); // Free the memory allocated for the removed node
            // pthread_mutex_unlock(&mutex);
            return; // Node removed, exit the function
        }
        // Move to the next node in the list
        prev = current;
        current = current->next;
    }
    //pthread_mutex_unlock(&mutex);
}

char* build_query_list(ClientNode *head){
    char* result = malloc(MAX_USERS * (MAX_NAME + MAX_DATA + 3));
    result[0] = '\0';
    ClientNode *current = head;
    while (current != NULL) {
        strcat(result, "User: ");
        strcat(result, current->client.user.username);
        strcat(result, " ");
        if(current->client.in_session){
            strcat(result, "Session: ");
            strcat(result, current->client.session_id);
        }
        strcat(result, "\n"); 
        current = current->next;
    }
    return result;
}