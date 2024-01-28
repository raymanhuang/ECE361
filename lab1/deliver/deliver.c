#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server address> <server port number>\n", argv[0]);
        return 1;
    }

    const char *server_address = argv[1];
    unsigned short server_port = (unsigned short)atoi(argv[2]);

    // Step 1: Ask the user to input a message
    printf("Please input a message (ftp <file name>):\n");
    char input[BUFFER_SIZE];
    fgets(input, BUFFER_SIZE, stdin);

    // Check format and file existence
    char *keyword = strtok(input, " ");
    char *filename = strtok(NULL, " \n");
    if (keyword == NULL || filename == NULL || strcmp(keyword, "ftp") != 0) {
        fprintf(stderr, "Invalid input format. Please use: ftp <file name>\n");
        return 1;
    }

    // Step 2: Check the existence of the file
    struct stat file_stat;
    if (stat(filename, &file_stat) != 0) {
        perror("File does not exist or cannot be accessed");
        return 1;
    }

    // File exists, proceed with sending message to the server
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Cannot create socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_address, &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        return 1;
    }

    // Step 3: Send a message "ftp" to the server
    const char *ftp_message = "ftp";
    if (sendto(sockfd, ftp_message, strlen(ftp_message), 0, 
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto failed");
        return 1;
    }

    // Receive a message from the server
    char buffer[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    ssize_t message_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, 
                                   (struct sockaddr *)&from_addr, &from_len);
    if (message_len < 0) {
        perror("recvfrom failed");
        return 1;
    }

    buffer[message_len] = '\0';  // Null-terminate the received message

    // Step 4: Check server's response
    if (strcmp(buffer, "yes") == 0) {
        printf("A file transfer can start.\n");
    } else {
        printf(stderr, "Server response not as expected.\n");
        return 1;
    }

    close(sockfd);
    return 0;
}
