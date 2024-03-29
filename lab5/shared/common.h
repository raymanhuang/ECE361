// server.h or common.h
#ifndef COMMON_H
#define COMMON_H

// Function declarations, type definitions, macros, etc.
#define MAX_DATA 1024
#define MAX_NAME 128
#define MAX_MESSAGE_SIZE = MAX_DATA + MAX_NAME

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>  // For atoi
#include <unistd.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>

typedef struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
} message;

typedef struct {
    unsigned int type;
    unsigned int length; // Length of the message data (excluding the header)
} message_header;

typedef enum {
    LOGIN,
    LO_ACK,
    LO_NAK,
    EXIT,
    JOIN,
    JN_ACK,
    JN_NAK,
    LEAVE_SESS,
    NEW_SESS,
    NS_ACK,
    NS_NAK,
    MESSAGE,
    PRIVATE_MSG,
    QUERY,
    QU_ACK,
    MSG_NAK,
    LGO_ACK
} type;

void serialize_message_with_header(const message *msg, unsigned char *buffer);
void deserialize_message(const unsigned char *buffer, message *msg, unsigned int data_length);
void send_message(int sockfd, message *msg);
void receive_message(int sockfd, message* to_be_received);

#endif // COMMON_H