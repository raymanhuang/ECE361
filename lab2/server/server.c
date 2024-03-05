#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <netinet/in.h>
#include <stdlib.h>  // For atoi
#include <unistd.h>  // For close

#define BUFFER_SIZE 2048

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

int parse_packet(const char *buffer, ssize_t buffer_len, Packet *packet) {
    // Temporary variables to hold the parsed integers
    unsigned int total_frag, frag_no, size;
    char filename[FILENAME_MAX]; // Temporary buffer for the filename

    // Parse the header
    int scanned = sscanf(buffer, "%u:%u:%u:%[^:]:", &total_frag, &frag_no, &size, filename);
    if (scanned != 4) { // Ensure that exactly four items were scanned
        fprintf(stderr, "Failed to parse packet header\n");
        return -1; // Error code for parsing failure
    }

    // Calculate the length of the header to find the start of the file data
    size_t header_length = snprintf(NULL, 0, "%u:%u:%u:%s:", total_frag, frag_no, size, filename);

    // Ensure we don't read past the buffer
    if (header_length >= (size_t)buffer_len || header_length + size > (size_t)buffer_len) {
        fprintf(stderr, "Packet data exceeds buffer length\n");
        return -1;
    }

    // Allocate memory for the filename in the packet struct and copy it
    packet->filename = strdup(filename);
    if (!packet->filename) {
        perror("Failed to allocate memory for filename");
        return -1; // Error code for memory allocation failure
    }

    // Copy the file data from the buffer to the packet's filedata
    const char *filedata_start = buffer + header_length;
    memcpy(packet->filedata, filedata_start, size);

    // Set the rest of the packet fields
    packet->total_frag = total_frag;
    packet->frag_no = frag_no;
    packet->size = size;

    return 0; // Success
}



int main(int argc, char* argv[]){
    srand(time(NULL)); 
    if (argc != 2) {
        printf("Usage: %s <UDP listen port>\n", argv[0]);
        return 1;
    }

    unsigned short port = (unsigned short)atoi(argv[1]);
    if (port == 0) {
        printf("Invalid Port Number!\n");
        return 1;
    }

    // Creates the socket (domain, type, protocol)
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); // host to network short
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // host to network long

    // Checks the return value of bind
    if(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("Try again with a different port number!\n");
        return 1;
    }

    char buffer[BUFFER_SIZE];
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    ssize_t packet_len;

    while(1){
            // receive the first packet
        memset(buffer, 0, BUFFER_SIZE);
        packet_len = recvfrom(server_socket, buffer, BUFFER_SIZE,
                        0, (struct sockaddr*)&client_addr, &client_addr_len);
        if (packet_len == -1) {
            perror("recvfrom failed");
            // Handle error such as by exiting or attempting to receive again
            close(server_socket);
            return 1;
        }


        printf("packet_len = %d\n", packet_len);

        Packet packet;
        int parse_result = parse_packet(buffer, BUFFER_SIZE, &packet);

        // Check the result of parse_packet
        if (parse_result == -1) {
            printf("Failed to parse the packet\n");
            // Handle parsing error, potentially by cleaning up and exiting
            close(server_socket);
            return 1;
        }

        // Print the non-binary parts of the packet
        printf("Packet total fragments: %u\n", packet.total_frag);
        printf("Packet fragment number: %u\n", packet.frag_no);
        printf("Packet size: %u\n", packet.size);
        printf("Packet filename: %s\n", packet.filename);

        // Print the binary data in hexadecimal
        // printf("Packet filedata:\n");
        // for (unsigned int i = 0; i < packet.size; i++) {
        //     // Print each byte as a two-digit hexadecimal number
        //     printf("%02X ", (unsigned char)packet.filedata[i]);
        // }
        // printf("\n");   

        // create the file and write the first packets data
        unsigned int current_packet = 0;
        int total_frags = packet.total_frag;
        FILE* file_stream;
        if(packet.frag_no == 0){
            file_stream = fopen(packet.filename, "wb");
            if (file_stream == NULL) {
                printf("im in here 1\n");
                perror("Error opening file to write");
                free(packet.filename); 
                close(server_socket);
                return 1;
            }
            size_t bytes_written = fwrite(packet.filedata, sizeof(char), packet.size, file_stream);
            if (bytes_written < packet.size) {
                printf("im in here 2\n");
                fprintf(stderr, "Failed to write all data to the file\n");
                fclose(file_stream); // Close the file stream to flush and release the file
                free(packet.filename); // Don't forget to free dynamically allocated memory
                close(server_socket);
                return 1;
            }
            if (packet.total_frag == 1) {
                free(packet.filename);
                fclose(file_stream);
            }
        }
        // Send ACK for packet 0
        AckPacket ack;
        ack.ack_frag_no = htonl(current_packet);
        ssize_t sent_bytes = sendto(server_socket, &ack, sizeof(ack), 0, 
                                    (struct sockaddr *)&client_addr, client_addr_len);
        if (sent_bytes != sizeof(ack)) {
            printf("Failed to send ACK\n");
            fclose(file_stream);
            return 1;
        }
        // do the same for the remaining packets
        for(int i = 1; i < total_frags; i++){
            memset(buffer, 0, BUFFER_SIZE);
            memset(packet.filedata, 0, sizeof(packet.filedata));
            packet_len = recvfrom(server_socket, buffer, BUFFER_SIZE,
                        0, (struct sockaddr*)&client_addr, &client_addr_len);
            if (packet_len == -1) {
                perror("recvfrom failed");
                close(server_socket);
                return 1;
            }

            // Simulate packet loss: 1% chance to drop the packet
            if ((rand() % 100) < 5) {  // Adjust the 5 to change the drop rate
                printf("Simulating packet drop (not sending ACK for packet %d)\n", current_packet);
                i--;  // Stay on the same packet number, waiting for retransmission
                continue;  // Skip sending ACK for this packet
            }

            parse_result = parse_packet(buffer, packet_len, &packet);
            // Check the result of parse_packet
            if (parse_result == -1) {
                printf("Failed to parse the packet\n");
                // Handle parsing error, potentially by cleaning up and exiting
                close(server_socket);
                return 1;
            }
            current_packet = packet.frag_no;
            size_t bytes_written = fwrite(packet.filedata, sizeof(char), packet.size, file_stream);
            if (bytes_written < packet.size) {
                printf("Failed to write all data to the file\n");
                fclose(file_stream); // Close the file stream to flush and release the file
                free(packet.filename); 
                close(server_socket);
                return 1;
            }
            free(packet.filename);
            // send ack for i'th packet
            ack.ack_frag_no = htonl(current_packet);
            sent_bytes = sendto(server_socket, &ack, sizeof(ack), 0, 
                                    (struct sockaddr *)&client_addr, client_addr_len);
            if (sent_bytes != sizeof(ack)) {
                printf("Failed to send ACK\n");
                fclose(file_stream);
                return 1;
            }
        }
        // close file stream
        fclose(file_stream);
    }
    close(server_socket);
    return 0;
}