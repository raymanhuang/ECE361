#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#define BUFFER_SIZE 1024

int main(int argc, char* argv[]){

    if (argc != 2) {
        printf("Usage: %s <UDP listen port>\n", argv[0]);
        return 1;
    }

    unsigned short port = (unsigned short)atoi(argv[1]);
    if (port == 0) {
        printf("Invalid Port Number!\n");
        return 1;
    }

    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("Try again with a different port number!\n");
        return 1;
    }

    char buffer[BUFFER_SIZE];
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while(1){
        ssize_t msg_length = recvfrom(server_socket, buffer, BUFFER_SIZE,
                                      0, (struct sockaddr*)&client_addr, &client_addr_len);
        if (msg_length < 0) {
            printf("recvfrom failed\n");
            break;
        }
        buffer[msg_length] = '\0';
        // Check if the message is "ftp"
        const char* response = (strcmp(buffer, "ftp") == 0) ? "yes" : "no";

        // Send response back to the client
        ssize_t sent_len = sendto(server_socket, response, strlen(response), 0, 
                                  (struct sockaddr*) &client_addr, client_addr_len);
        if (sent_len < 0) {
            printf("sendto failed\n");
            break;
        }
    }
    close(server_socket);
    return 0;
}