#ifndef LAB_H
#define LAB_H

#include <stddef.h>
#include <stdint.h>
#include <netdb.h>

#define PAYLOAD_SIZE 1024

#define TIMEOUT 250

#define RELAY_PORT 4250

typedef enum
{
    DATA,
    ACK,
    FIN
} PROTOCOL_TYPE;

// Header structure for packet
typedef struct
{
    PROTOCOL_TYPE pack_type;
    uint16_t checksum;
    uint16_t seq_num;
    size_t data_len;
    char data[PAYLOAD_SIZE];
} header;

typedef struct
{
    char *session;
    char *relay;
    char *file_name;
    int port;
} CLIENT_ARGUMENT;

typedef struct
{
    char *session;
    char *relay;
    char *file_name;
    int window;
    int timeout;
    double loss;
    double corrupt;
    double dup;
    int port;
} SERVER_ARGUMENT;

CLIENT_ARGUMENT *parse_cl_opt(int argc, char *const argv[]);

int init_client(CLIENT_ARGUMENT *client);
int register_client(int fd, CLIENT_ARGUMENT *client);

SERVER_ARGUMENT *parse_ser_opt(int argc, char *const argv[]);

int init_server(SERVER_ARGUMENT *client);
int register_server(int fd, SERVER_ARGUMENT *server);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#endif // LAB_H
