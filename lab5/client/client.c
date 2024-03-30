#include "common.h"
#include "client.h"

typedef struct {
    bool running;
    int sockfd;
    bool logged_in;
    char client_name[MAX_NAME];
} Global;

Global g;

void *listen_for_server_messages(void *arg) {
    // ThreadData *data = (ThreadData *)arg;
    // printf("listening from server...\n");
    while (g.running) {
        // Listen for messages from the server
        // For example, using recv on a socket
        message msg;
        receive_message(g.sockfd, &msg);
        if (msg.type == LO_ACK){
            printf("%s\n", msg.data);
            g.logged_in = true;
        } 
        else if (msg.type == REG_ACK){
            printf("%s\n", msg.data);
            g.logged_in = true;
        }
        else if (msg.type == LO_NAK){
            printf("%s\n", msg.data);
            return NULL;
        }
        else if (msg.type == REG_NAK){
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
        else if (msg.type == PRIVATE_MSG) {
            char* colon_ptr = strchr((char*)msg.data, ':');
            if (colon_ptr) {
                *colon_ptr = '\0';  // Terminate the sender's username string
                printf("Private message from %s: %s\n", msg.source, colon_ptr + 1);  // Display the message
            } else {
                printf("Invalid private message format received.\n");
            }
        }
        else if(msg.type == MSG_NAK){
            printf("%s\n", msg.data);
        }
        else if(msg.type == LGO_ACK){
            close(g.sockfd);
            g.logged_in = false;
            printf("server listener ending\n");
            return NULL;
        }
        else if(msg.type == QU_ACK){
            printf("%s\n", msg.data);
        }
    }
    printf("Goodbye!\n");
    return NULL;
}

void handle_sigint(int sig) {
    // Perform the same logic as /quit command
    printf("\nHandling Ctrl+C (SIGINT)\n");

    if (g.logged_in) {
        printf("Logging out first...\n");
        message logout_message;
        logout_message.type = EXIT;
        logout_message.size = strlen("logout_req");
        strcpy((char*)logout_message.source, g.client_name);
        strcpy((char*)logout_message.data, "logout_req");
        send_message(g.sockfd, &logout_message);
        g.running = false; // Stop the main loop
        g.logged_in = 0; // Mark as logged out
        close(g.sockfd); // Close the connection
        exit(0);
    }
    message exit_message;
    exit_message.type = EXIT;
    exit_message.size = strlen("exiting");
    strcpy((char*)exit_message.source, "stub");
    strcpy((char*)exit_message.data, "exiting");
    send_message(g.sockfd, &exit_message);
    g.running = false; // Stop the main loop
    g.logged_in = 0; // Mark as logged out
    close(g.sockfd); // Close the connection
    exit(0); // Exit the application
}

int main(int argc, char **argv) {

    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sa.sa_flags = 0; // or SA_RESTART to restart system calls
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    printf("Welcome to ECE361 Text Conferencing!\n");
    // int sockfd;
    g.running = true;
    g.logged_in = false;
    // char client_name[MAX_NAME];
    while(g.running){
        // displayPrompt();
        char* args[MAX_ARGS + 1];
        Command command = handleCommand(args);
        if(command == LOG_IN && g.logged_in == false){
            // Connect to server
            struct sockaddr_in server_addr;
            g.sockfd = socket(AF_INET, SOCK_STREAM, 0);
            // thread_args.sockfd = sockfd;
            if(g.sockfd < 0){
                perror("Socket creation failed");
                exit(EXIT_FAILURE);
            }
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(atoi(args[4])); // Server port number
            server_addr.sin_addr.s_addr = inet_addr(args[3]); // Server IP address

            if (connect(g.sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                perror("Connection failed");
                exit(EXIT_FAILURE);
            }

            // Create thread
            pthread_t server_thread;
            if(pthread_create(&server_thread, NULL, listen_for_server_messages, NULL)){
                perror("Failed to create thread");
                exit(EXIT_FAILURE);
            }

            // Send a message to the server with login info
            message login_msg;
            login_msg.type = LOGIN;
            login_msg.size = strlen(args[2]);
            strcpy((char*)login_msg.source, args[1]);
            strcpy(g.client_name, args[1]);
            strcpy((char*)login_msg.data, args[2]);
            send_message(g.sockfd, &login_msg);
        }
        else if(command == REGISTER){
            printf("made it here\n");
            if(g.logged_in == true){
                printf("Please log out first!\n");
            } else {
                // Connect to server
                struct sockaddr_in server_addr;
                g.sockfd = socket(AF_INET, SOCK_STREAM, 0);
                // thread_args.sockfd = sockfd;
                if(g.sockfd < 0){
                    perror("Socket creation failed");
                    exit(EXIT_FAILURE);
                }
                memset(&server_addr, 0, sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(atoi(args[4])); // Server port number
                server_addr.sin_addr.s_addr = inet_addr(args[3]); // Server IP address

                if (connect(g.sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                    perror("Connection failed");
                    exit(EXIT_FAILURE);
                }            
                // Create thread
                pthread_t server_thread;
                if(pthread_create(&server_thread, NULL, listen_for_server_messages, NULL)){
                    perror("Failed to create thread");
                    exit(EXIT_FAILURE);
                }
                // Send a message to the server with login info
                message register_msg;
                register_msg.type = REG;
                register_msg.size = strlen(args[2]);
                strcpy((char*)register_msg.source, args[1]);
                strcpy(g.client_name, args[1]); // username
                strcpy((char*)register_msg.data, args[2]); // password
                send_message(g.sockfd, &register_msg);
            }
        }
        else if(command == CREATE_SESSION){
            if(g.logged_in == false){
                printf("Please log in first!\n");
                continue;
            }
            message create_session_msg;
            create_session_msg.type = NEW_SESS;
            create_session_msg.size = strlen(args[1]);
            strcpy((char*)create_session_msg.source, g.client_name);
            strcpy((char*)create_session_msg.data, args[1]);
            send_message(g.sockfd, &create_session_msg);
        }
        else if(command == JOIN_SESSION){
            if(g.logged_in == false){
                printf("Please log in first!\n");
                continue;
            }
            message join_session_msg;
            join_session_msg.type = JOIN;
            join_session_msg.size = strlen(args[1]);
            strcpy((char*)join_session_msg.source, g.client_name);
            strcpy((char*)join_session_msg.data, args[1]);
            send_message(g.sockfd, &join_session_msg);
        }
        else if(command == LEAVE_SESSION){
            if(g.logged_in == false){
                printf("Please log in first!\n");
                continue;
            }
            message leave_session_msg;
            leave_session_msg.type = LEAVE_SESS;
            leave_session_msg.size = strlen("leave_req");
            strcpy((char*)leave_session_msg.source, g.client_name);
            strcpy((char*)leave_session_msg.data, "leave_req");
            send_message(g.sockfd, &leave_session_msg);
        }
        else if(command == TEXT){
            if(g.logged_in == false){
                printf("Please log in first!\n");
                continue;
            }
            message text_message;
            text_message.type = MESSAGE;
            text_message.size = strlen(args[0]);
            strcpy((char*)text_message.source, g.client_name);
            strcpy((char*)text_message.data, args[0]);
            send_message(g.sockfd, &text_message);
        }
        else if(command == PRIVATEMSG) {
            if(g.logged_in == false){
                printf("Please log in first!\n");
                continue;
            }

            // concatenate all parts of the message after the recipient username
            char private_message_text[MAX_DATA] = {0};
            for (int j = 2; args[j] != NULL; j++) {
                strcat(private_message_text, args[j]);
                if (args[j + 1] != NULL) strcat(private_message_text, " "); // add spacing between words
            }

            // construct the full private message text, name + msg
            char pm[MAX_DATA];
            strcpy(pm, args[1]); 
            strcat(pm, ":");
            strcat(pm, private_message_text); 

            // Construct the message
            message text_message;
            text_message.type = PRIVATE_MSG;
            strcpy((char*)text_message.source, g.client_name);
            strcpy((char*)text_message.data, pm);
            text_message.size = strlen(pm); 

            send_message(g.sockfd, &text_message);
        }
        else if(command == LOGOUT){
            if(g.logged_in == false){
                printf("Please log in first!\n");
                continue;
            }
            message logout_message;
            logout_message.type = EXIT;
            logout_message.size = strlen("logout_req");
            strcpy((char*)logout_message.source, g.client_name);
            strcpy((char*)logout_message.data, "logout_req");
            send_message(g.sockfd, &logout_message);
        }
        else if(command == QUIT){
            if(g.logged_in == false){
                printf("Quitting... goodbye\n");
                break;
            } else {
                printf("Logging out first...\n");
                message logout_message;
                logout_message.type = EXIT;
                logout_message.size = strlen("logout_req");
                strcpy((char*)logout_message.source, g.client_name);
                strcpy((char*)logout_message.data, "logout_req");
                send_message(g.sockfd, &logout_message);
                printf("Quitting... goodbye\n");
            }
        }
        else if(command == LIST){
            if(g.logged_in == false){
                printf("Please log in first!\n");
                continue;
            } else {
                message list_message;
                list_message.type = QUERY;
                list_message.size = strlen("list_req");
                strcpy((char*)list_message.source, g.client_name);
                strcpy((char*)list_message.data, "list_req");
                send_message(g.sockfd, &list_message);
            }
        }
        else if(command == INVALID_COMMAND){
            printf("Invalid command entered!\n");
        }
        for (int i = 0; args[i] != NULL; i++) {
            free(args[i]);
        }
    }
    close(g.sockfd);
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

            // Check for the /login command and ensure it has exactly 4 arguments (5 tokens in total)
            if (strcmp(args[0], "/login") == 0) {
                if (i != 5) { // Including the command itself, there should be 5 tokens
                    printf("Error: /login requires 4 arguments (username, password, server_ip, server_port).\n");
                    return INVALID_COMMAND;
                }
                return LOG_IN;
            } else if (strcmp(args[0], "/logout") == 0) { // done
                return LOGOUT;
            } else if (strcmp(args[0], "/joinsession") == 0) { // done
                return JOIN_SESSION;
            } else if (strcmp(args[0], "/leavesession") == 0) { // done
                return LEAVE_SESSION;
            } else if (strcmp(args[0], "/createsession") == 0) { // done
                return CREATE_SESSION;
            } else if (strcmp(args[0], "/list") == 0) { // done
                return LIST;
            } else if (strcmp(args[0], "/quit") == 0) { // done
                return QUIT;
            } else if (strcmp(args[0], "/pm") == 0) { // private messaging
                // checking that it has at least 3 tokens (command, recipient, msg)
                if (i < 3) { 
                    printf("/pm requires (user, message)\n");
                    return INVALID_COMMAND;
                }
                return PRIVATEMSG;
            } else if (strcmp(args[0], "/register") == 0){
                if (i != 5) { // Including the command itself, there should be 5 tokens
                    printf("Error: /register requires 4 arguments (username, password, server_ip, server_port).\n");
                    return INVALID_COMMAND;
                }
                return REGISTER;
            }
             else {        // done
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

