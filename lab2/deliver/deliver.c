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
#include <time.h>

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

char* serialize_packet(const Packet* pkt, size_t* out_size, size_t* out_header_length) {
    // Calculate the length needed for the header
    size_t header_length = snprintf(NULL, 0, "%u:%u:%u:%s:", pkt->total_frag, pkt->frag_no, pkt->size, pkt->filename);
    
    // Calculate the total size needed: header + data
    size_t packet_size = header_length + pkt->size;

    // Allocate memory for the serialized packet
    char* buffer = malloc(packet_size + 1); // +1 for the null terminator, if needed
    if (!buffer) {
        perror("Failed to allocate memory for packet serialization");
        return NULL;
    }

    // Write the header into the buffer
    int header_written = snprintf(buffer, header_length + 1, "%u:%u:%u:%s:", pkt->total_frag, pkt->frag_no, pkt->size, pkt->filename);
    if (header_written < 0 || (size_t)header_written != header_length) {
        free(buffer);
        return NULL;
    }

    // Copy the file data directly after the header
    memcpy(buffer + header_length, pkt->filedata, pkt->size);

    // Set the output size for the caller to know how many bytes to send
    *out_size = packet_size;

    // Also return the header length
    *out_header_length = header_length;

    // Return the serialized packet buffer
    return buffer;
}

unsigned int find_total_frags(off_t file_size, int max_packet_size){
    if (file_size % max_packet_size == 0){
        return file_size / max_packet_size;
    }
    else {
        return file_size / max_packet_size + 1;
    }
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
    unsigned char* file_contents = malloc(file_stat.st_size * sizeof(unsigned char));
    if (!file_contents) {
        perror("Memory allocation failed");
        fclose(file);
        return 1;
    }  
    
    unsigned int total_fragments = find_total_frags(file_stat.st_size, 1000);
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

    printf("Allocating memory for %u fragments\n", total_fragments);
    Packet* packets = malloc(total_fragments * sizeof(Packet));
    if (packets == NULL) {
        perror("Failed to allocate memory for packets");
        return 1; // Or handle the error as appropriate
    }
    for (int i = 0; i < total_fragments; i++) {
        // printf("i = %d\n", i);
        packets[i].total_frag = total_fragments;
        packets[i].frag_no = i;
        // printf("Set total_frag and frag_no for packet[%d]\n", i);

        packets[i].filename = strdup(filename);
        if (packets[i].filename == NULL) {
            printf("Failed to allocate memory for packets[%d].filename\n", i);
            return 1;
        }
        // printf("Allocated filename for packet[%d]: '%s'\n", i, packets[i].filename);

        if (i < total_fragments - 1 || file_stat.st_size % 1000 == 0) {
            packets[i].size = 1000;
        } else {
            packets[i].size = file_stat.st_size % 1000; // Last fragment size
        }
        // printf("Packet[%d] size set to: %u\n", i, packets[i].size);

        // printf("Attempting to copy data to packet[%d].filedata, size: %u\n", i, packets[i].size);
        memcpy(packets[i].filedata, file_contents + (i * 1000), packets[i].size);
        // printf("memcpy completed for packet[%d]\n", i);

        // printf("Data copied for packet[%d]: ", i);
        // for (unsigned int j = 0; j < packets[i].size; j++) {
        //     printf("%02X ", packets[i].filedata[j]);
        // }
        // printf("\n");
    }
    printf("Exited packet processing loop\n");

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

    clock_t start_time, end_time;
    for(int i = 0; i < total_fragments; i++) {
        // printf("i = %d\n", i);
        // Serialize the packet into a correctly formatted string.
        size_t serialized_size = 0;
        size_t header_length = 0;
        char *serialized_packet = serialize_packet(&packets[i], &serialized_size, &header_length);
   
        
        if (!serialized_packet) {
            perror("Failed to serialize packet");
            return 1;
        }
        // printf("Serialized packet header: %.*s\n", (int)header_length, serialized_packet);

        // printf("Serialized packet binary data:\n");
        // for (size_t i = header_length; i < serialized_size; ++i) {
        //     printf("%02X ", (unsigned char)serialized_packet[i]);
        //     }
        // printf("\n");

        // Send the packet
        if(i == 0){
            start_time = clock();
        }
        if (sendto(sockfd, serialized_packet, serialized_size, 0, 
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
        // printf("waiting for ack...\n");
        ssize_t ack_len = recvfrom(sockfd, &ack, sizeof(ack), 0, 
                                    (struct sockaddr*)&from_addr, &from_len);
        if(i == 0){
            end_time = clock();
            clock_t RTT = end_time - start_time;
            printf("THE RTT IS: %ld ms\n", RTT);
        }
        if (ack_len < 0){
            perror("recvfrom failed");
            break;
        } else if (ack_len == sizeof(ack) && ntohl(ack.ack_frag_no) == packets[i].frag_no){
            // printf("ACK received for fragment %u\n", packets[i].frag_no);
        } else {
            printf("Invalid ACK\n");
            break;
        }
    }
    printf("made it out\n");
    // Step 4: clean up 
    for(int i = 0; i < total_fragments; i++){
        free(packets[i].filename);
    }
    free(packets);
    free(file_contents);
    close(sockfd);
    return 0;
}
