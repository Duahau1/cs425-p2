#ifndef LAB_H
#define LAB_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stddef.h>
#include <stdint.h>
#include <netdb.h>
#include <poll.h>
#include <limits.h>

#define MAX_SIZE_OF_FILE (16U * 1024U * 1024U)

#define PAYLOAD_SIZE 1024U
#define HEADER_SIZE 10U

#define TIMEOUT 250

#define RELAY_PORT 4250

#define REGISTER_MAX_ATTEMPT 5

#define WINDOW_MAX 64U

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
} packet_header;

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

typedef struct
{
    uint32_t expected;
    int finished;
    int64_t last_valid_ms;
    int64_t linger_time_ms;
} client_state;

CLIENT_ARGUMENT *parse_cl_opt(int argc, char *const argv[]);

int init_client(CLIENT_ARGUMENT *client);
int register_client(int fd, CLIENT_ARGUMENT *client);
int process(int fd, CLIENT_ARGUMENT *client);

typedef struct
{
    const uint8_t *data;
    size_t size;
    uint32_t total_chunks;
    uint32_t base;
    uint32_t next;
    int64_t limit_ms;
    unsigned window;
    unsigned timeout;
    int finished;
    int failed;

    packet_header standby[WINDOW_MAX];
} server_state;

SERVER_ARGUMENT *parse_ser_opt(int argc, char *const argv[]);

int init_server(SERVER_ARGUMENT *client);
int register_server(int fd, SERVER_ARGUMENT *server);
int publish(int fd, SERVER_ARGUMENT *server);
int process(int fd, CLIENT_ARGUMENT *client);
int get_remaining_timeout_ms(const client_state *state, int64_t now);
int parse_incoming_packet(const uint8_t *packetPayload, size_t size, packet_header *outputHeader);
int consume(int sock_fd, client_state *client_state, packet_header *incoming_packet, FILE *opened_file);
int send_ack(int sock_fd, packet_header *incoming_packet);
size_t populate_transmission_window(server_state *state, int64_t now_ms, packet_header *out);
int flush(int fd, const packet_header *packets, size_t count);

#endif // LAB_H
