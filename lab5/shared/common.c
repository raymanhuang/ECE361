#include "common.h"

void serialize_message_with_header(const message *msg, unsigned char *buffer) {
    message_header header;
    header.type = htonl(msg->type);
    header.length = htonl(msg->size);

    memcpy(buffer, &header, sizeof(header));
    buffer += sizeof(header);
    // printf("size of header: %d\n", sizeof(header));
    memcpy(buffer, msg->source, MAX_NAME);
    // printf("serializing name: %s\n", msg->source);
    buffer += MAX_NAME;
    // printf("serializing data: %s\n", msg->data);
    memcpy(buffer, msg->data, msg->size);
}

void deserialize_message(const unsigned char *buffer, message *msg, unsigned int data_length) {
   
    memcpy(msg->source, buffer, MAX_NAME);
    buffer += MAX_NAME;
    // printf("deserialized name: %s\n", msg->source);
    unsigned int actual_data_length = data_length;
    if (actual_data_length > MAX_DATA) {
        actual_data_length = MAX_DATA; // Truncate if necessary
    }

    memcpy(msg->data, buffer, actual_data_length);
    if (actual_data_length < MAX_DATA) {
        msg->data[actual_data_length] = '\0'; 
    } else {
        msg->data[MAX_DATA - 1] = '\0'; 
    }
    msg->size = actual_data_length; // Set the actual size of the data
    // printf("message deserialized\n");
}

void send_message(int sockfd, message *msg) {
    unsigned char buffer[sizeof(message_header) + MAX_NAME + MAX_DATA];
    serialize_message_with_header(msg, buffer);

    int total_size = sizeof(message_header) + MAX_NAME + msg->size;
    int bytes_sent = 0;

    while (bytes_sent < total_size) {
        int result = send(sockfd, buffer + bytes_sent, total_size - bytes_sent, 0);
        if (result < 0) {
            perror("Error sending message");
            // Handle error
            break;
        }
        bytes_sent += result;
    }

    if (bytes_sent == total_size) {
        //printf("Message sent successfully\n");
    } else {
        printf("Only sent %d bytes of %d\n", bytes_sent, total_size);
        // Handle partial send
    }
}

void receive_message(int sockfd, message* to_be_received) {
    // printf("IM RECEIVING!\n");
    unsigned char header_buffer[sizeof(message_header)];
    int bytes_received = recv(sockfd, header_buffer, sizeof(message_header), 0);

    if (bytes_received <= 0) {
        // Connection closed or error occurred
        if (bytes_received == 0) {
            printf("Connection closed by server.\n");
        } else {
            perror("recv failed");
        }
        return;
    }

    // printf("Received message header: %d bytes\n", bytes_received);
    message_header header;
    memcpy(&header, header_buffer, sizeof(header));
    header.type = ntohl(header.type);
    header.length = ntohl(header.length);
    // printf("%d\n", header.length);

    unsigned char *message_data = (unsigned char *)malloc(MAX_NAME + header.length);
    int total_received = 0;
    while (total_received < header.length) {
        bytes_received = recv(sockfd, message_data + total_received, MAX_NAME + header.length - total_received, 0);
        if (bytes_received <= 0) {
            // Connection closed or error occurred
            if (bytes_received == 0) {
                printf("Connection closed by server.\n");
            } else {
                perror("recv failed");
            }
            free(message_data);
            return;
        }
        total_received += bytes_received;
    }
    // printf("Total received bytes of message: %d\n", total_received);
    to_be_received->type = header.type;
    deserialize_message(message_data, to_be_received, header.length);
    free(message_data);
}
