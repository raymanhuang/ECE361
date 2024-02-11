#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <math.h>

#define BUFFER_SIZE 1024

typedef struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
} Packet;

typedef struct ack_packet {
    unsigned int ack_frag_no;
} AckPacket;

char* serialize_packet(const Packet* pkt){
    size_t file_name_length = strlen(pkt->filename);
    size_t header_length = snprintf(NULL, 0, "%u:%u:%u:%s:", pkt->total_frag, pkt->frag_no, pkt->size, pkt->filename);
    size_t packet_size = header_length + pkt->size;

    char* buffer = malloc(packet_size); 
    if (!buffer) {
        perror("Failed to allocate memory for packet serialization");
        return NULL;
    }

    int header_size = snprintf(buffer, header_length + 1, "%u:%u:%u:%s:", pkt->total_frag, pkt->frag_no, pkt->size, pkt->filename);

    if (header_size != header_length) {
        free(buffer);
        return NULL;
    }

    memcpy(buffer + header_length, pkt->filedata, pkt->size);


    return buffer;

}

int main(int argc, char *argv[]) {
    // Checking the length of the arguement
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server address> <server port number>\n", argv[0]);
        return 1;
    }

    const char *server_address = argv[1];
    unsigned short server_port = (unsigned short)atoi(argv[2]); // ascii to int

    // Step 1: Ask the user to input a message
    printf("Please input a message (ftp <file name>):\n");
    char input[BUFFER_SIZE];
    fgets(input, BUFFER_SIZE, stdin);

    // Check format and file existence
    char *keyword = strtok(input, " ");
    char *filename = strdup(strtok(NULL, " \n"));
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

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }
    unsigned char* file_contents = malloc(file_stat.st_size);
    if (!file_contents) {
        perror("Memory allocation failed");
        fclose(file);
        return 1;
    }  
    
    unsigned int total_fragments = ceil(file_stat.st_size / 1000.0);
    printf("file size: %u\n", total_fragments);

    size_t bytes_read = fread(file_contents, sizeof(unsigned char), file_stat.st_size, file);
    printf("bytes read %d\n", bytes_read);
    if (bytes_read < file_stat.st_size) {
        if (feof(file)) {
            printf("Premature end of file.\n");
        } else if (ferror(file)) {
            perror("Error reading from file");
        }
        free(file_contents);
        fclose(file);
        return 1;
    }
    fclose(file);

    for(size_t i = 0; i < file_stat.st_size; i++) {
    printf("%c", file_contents[i]); // Print each byte in hex format
    }
    printf("\n");

    Packet* packets = malloc(total_fragments * sizeof(Packet));
    
    for(int i = 0; i < total_fragments; i++){
        packets[i].total_frag = total_fragments;
        packets[i].frag_no = i;
        packets[i].filename = strdup(filename);
        if(i < total_fragments - 1 || file_stat.st_size % 1000 == 0){
            packets[i].size = 1000;
        } else {
            packets[i].size = file_stat.st_size % 1000; // Last fragment size
        }
        memcpy(packets[i].filedata, file_contents + (i * 1000), packets[i].size);
    }
    // File exists, proceed with sending file to the server
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Cannot create socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port); // port number (arg 2)
    // inet_pton converts string from printable form to "network" form
    if (inet_pton(AF_INET, server_address, &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        return 1;
    }

    // Step 3: Send the file to the server

    for(int i = 0; i < total_fragments; i++) {
        // Serialize the packet into a correctly formatted string.
        char *serialized_packet = serialize_packet(&packets[i]);
        if (!serialized_packet) {
            perror("Failed to serialize packet");
            return 1;
        }

        // Calculate the size of the packet
        size_t packet_size = strlen(packets[i].total_frag) + 1
                         + strlen(packets[i].frag_no) + 1
                         + strlen(packets[i].size) + 1
                         + strlen(packets[i].filename) + 1
                         + packets[i].size;

        // Send the packet
        if (sendto(sockfd, serialized_packet, packet_size, 0, 
                (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("sendto failed");
            free(serialized_packet);
            for(int j = 0; j <= i; j++){
                free(packets[j].filename);
            }
            free(packets);
            free(file_contents);
            close(sockfd);
            return 1;
        }
        // Free the serialized packet after sending
        free(serialized_packet);

        // Wait for ACK
        AckPacket ack;
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        ssize_t ack_len = recvfrom(sockfd, &ack, sizeof(ack), 0, 
                                    (struct sockaddr*)&from_addr, &from_len);
        if (ack_len < 0){
            perror("recvfrom failed");
            break;
        } else if (ack_len == sizeof(ack) && ntohl(ack.ack_frag_no) == packets[i].frag_no){
            printf("ACK received for fragment %u\n", packets[i].frag_no);
        } else {
            printf("Invalid ACK\n");
            break;
        }
    }

    // Step 4: clean up 
    for(int i = 0; i < total_fragments; i++){
        free(packets[i].filename);
    }
    free(packets);
    free(file_contents);
    close(sockfd);
    return 0;
}
