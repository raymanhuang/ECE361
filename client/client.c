#include "common.h"
#include "client.h"

typedef struct {
    bool running;
    int sockfd;
    bool logged_in;
} ThreadData;

void *listen_for_server_messages(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    // printf("listening from server...\n");
    while (data -> running) {
        // Listen for messages from the server
        // For example, using recv on a socket
        message msg;
        receive_message(data->sockfd, &msg);
        if (msg.type == LO_ACK){
            printf("%s\n", msg.data);
            data->logged_in = true;
        } 
        else if (msg.type == LO_NAK){
            printf("%s\n", msg.data);
            return NULL;
        }
        else if (msg.type == NS_NAK){
            printf("%s\n", msg.data);
        }
        else if (msg.type == NS_ACK){
            printf("%s\n", msg.data);
        }
        else if (msg.type == JN_ACK){
            printf("%s\n", msg.data);
        }
        else if (msg.type == JN_NAK){
            printf("%s\n", msg.data);
        }
        else if(msg.type == MESSAGE){
            printf("Message from %s: ", msg.source);
            printf("%s\n", msg.data);
        }
        else if(msg.type == MSG_NAK){
            printf("%s\n", msg.data);
        }
        else if(msg.type == LGO_ACK){
            close(data->sockfd);
            data->logged_in = false;
            return NULL;
        }
        else if(msg.type == QU_ACK){
            printf("%s\n", msg.data);
        }
    }
    printf("bye bye\n");
    return NULL;
}

int main(int argc, char **argv) {
    printf("Welcome to ECE361 Text Conferencing!\n");
    int sockfd;
    ThreadData thread_args;
    thread_args.running = true;
    thread_args.logged_in = false;
    char client_name[MAX_NAME];
    while(thread_args.running){
        // displayPrompt();
        char* args[MAX_ARGS + 1];
        Command command = handleCommand(args);
        if(command == LOG_IN && thread_args.logged_in == false){
            // Connect to server
            struct sockaddr_in server_addr;
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            thread_args.sockfd = sockfd;
            if(sockfd < 0){
                perror("Socket creation failed");
                exit(EXIT_FAILURE);
            }
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(atoi(args[4])); // Server port number
            server_addr.sin_addr.s_addr = inet_addr(args[3]); // Server IP address

            if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                perror("Connection failed");
                exit(EXIT_FAILURE);
            }

            // Create thread
            pthread_t server_thread;
            if(pthread_create(&server_thread, NULL, listen_for_server_messages, (void *)&thread_args)){
                perror("Failed to create thread");
                exit(EXIT_FAILURE);
            }

            // Send a message to the server with login info
            message login_msg;
            login_msg.type = LOGIN;
            login_msg.size = strlen(args[2]);
            strcpy((char*)login_msg.source, args[1]);
            strcpy(client_name, args[1]);
            strcpy((char*)login_msg.data, args[2]);
            send_message(sockfd, &login_msg);
        }
        else if(command == CREATE_SESSION){
            if(thread_args.logged_in == false){
                printf("Please log in first!\n");
                continue;
            }
            message create_session_msg;
            create_session_msg.type = NEW_SESS;
            create_session_msg.size = strlen(args[1]);
            strcpy((char*)create_session_msg.source, client_name);
            strcpy((char*)create_session_msg.data, args[1]);
            send_message(sockfd, &create_session_msg);
        }
        else if(command == JOIN_SESSION){
            if(thread_args.logged_in == false){
                printf("Please log in first!\n");
                continue;
            }
            message join_session_msg;
            join_session_msg.type = JOIN;
            join_session_msg.size = strlen(args[1]);
            strcpy((char*)join_session_msg.source, client_name);
            strcpy((char*)join_session_msg.data, args[1]);
            send_message(sockfd, &join_session_msg);
        }
        else if(command == LEAVE_SESSION){
            if(thread_args.logged_in == false){
                printf("Please log in first!\n");
                continue;
            }
            message leave_session_msg;
            leave_session_msg.type = LEAVE_SESS;
            leave_session_msg.size = strlen("leave_req");
            strcpy((char*)leave_session_msg.source, client_name);
            strcpy((char*)leave_session_msg.data, "leave_req");
            send_message(sockfd, &leave_session_msg);
        }
        else if(command == TEXT){
            if(thread_args.logged_in == false){
                printf("Please log in first!\n");
                continue;
            }
            message text_message;
            text_message.type = MESSAGE;
            text_message.size = strlen(args[0]);
            strcpy((char*)text_message.source, client_name);
            strcpy((char*)text_message.data, args[0]);
            send_message(sockfd, &text_message);
        }
        else if(command == LOGOUT){
            if(thread_args.logged_in == false){
                printf("Please log in first!\n");
                continue;
            }
            message logout_message;
            logout_message.type = EXIT;
            logout_message.size = strlen("logout_req");
            strcpy((char*)logout_message.source, client_name);
            strcpy((char*)logout_message.data, "logout_req");
            send_message(sockfd, &logout_message);
        }
        else if(command == QUIT){
            if(thread_args.logged_in == false){
                printf("Quitting... goodbye\n");
                break;
            } else {
                printf("Logging out first...\n");
                message logout_message;
                logout_message.type = EXIT;
                logout_message.size = strlen("logout_req");
                strcpy((char*)logout_message.source, client_name);
                strcpy((char*)logout_message.data, "logout_req");
                send_message(sockfd, &logout_message);
                printf("Quitting... goodbye\n");
            }
        }
        else if(command == LIST){
            if(thread_args.logged_in == false){
                printf("Please log in first!\n");
                continue;
            } else {
                message list_message;
                list_message.type = QUERY;
                list_message.size = strlen("list_req");
                strcpy((char*)list_message.source, client_name);
                strcpy((char*)list_message.data, "list_req");
                send_message(sockfd, &list_message);
            }
        }
        else if(command == INVALID_COMMAND){
            printf("Invalid command entered!\n");
        }
        for (int i = 0; args[i] != NULL; i++) {
            free(args[i]);
        }
    }
    close(sockfd);
    pthread_exit(NULL);
    return 0;
}

void displayPrompt() {
    printf("%s", CLIENT_PROMPT);
}

Command handleCommand(char** args) {
    char input[MAX_DATA];
    if (fgets(input, MAX_DATA, stdin) != NULL) {
        input[strcspn(input, "\n")] = '\0';
        if (input[0] == '/') {
            char* token = strtok(input, " ");
            int i = 0;
            while(token != NULL && i < MAX_ARGS){
                args[i++] = strdup(token);
                token = strtok(NULL, " ");
            }
            args[i] = NULL;
            if (strcmp(args[0], "/login") == 0) { // done
                return LOG_IN;
            } else if (strcmp(args[0], "/logout") == 0) { // done
                return LOGOUT;
            } else if (strcmp(args[0], "/joinsession") == 0) { // done
                return JOIN_SESSION;
            } else if (strcmp(args[0], "/leavesession") == 0) { // done
                return LEAVE_SESSION;
            } else if (strcmp(args[0], "/createsession") == 0) { // done
                return CREATE_SESSION;
            } else if (strcmp(args[0], "/list") == 0) { // IN PROGRESS
                return LIST;
            } else if (strcmp(args[0], "/quit") == 0) { // done
                return QUIT;
            } else {        // done
                return INVALID_COMMAND;
            }
        } else { // done
            args[0] = strdup(input);
            args[1] = NULL;
            return TEXT;
        }
    }
    return INVALID_COMMAND; 
}
