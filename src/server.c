#include "lab.h"
#include "utils.h"
#include <errno.h>

#ifdef TEST
extern ssize_t test_send(int socket_fd, const void *buffer, size_t length, int flags);
extern ssize_t test_recv(int socket_fd, void *buffer, size_t length, int flags);
extern int test_poll(struct pollfd *fds, nfds_t count, int timeout);
extern int test_socket(int domain, int type, int protocol);
extern int test_connect(int socket_fd, const struct sockaddr *addr, socklen_t addr_len);
extern int test_getaddrinfo(const char *node, const char *service,
                            const struct addrinfo *hints, struct addrinfo **result);
extern void test_freeaddrinfo(struct addrinfo *result);
extern int test_setsockopt(int socket_fd, int level, int option_name,
                           const void *option_value, socklen_t option_length);
extern int test_getopt(int argc, char *const argv[], const char *options);
extern void *test_malloc(size_t size);
#define send test_send
#define recv test_recv
#define poll test_poll
#define socket test_socket
#define connect test_connect
#define getaddrinfo test_getaddrinfo
#define freeaddrinfo test_freeaddrinfo
#define setsockopt test_setsockopt
#define getopt test_getopt
#define malloc test_malloc
#endif

SERVER_ARGUMENT *parse_ser_opt(int argc, char *const argv[])
{
    int option;
    SERVER_ARGUMENT *serverArgument = malloc(sizeof(*serverArgument));

    if (argc < 1 || argv == NULL || serverArgument == NULL)
    {
        free(serverArgument);
        return NULL;
    }

    serverArgument->session = NULL;
    serverArgument->relay = NULL;
    serverArgument->file_name = NULL;
    serverArgument->window = 8;
    serverArgument->timeout_ms = TIMEOUT;
    serverArgument->loss = 0;
    serverArgument->corrupt = 0;
    serverArgument->dup = 0;
    serverArgument->port = RELAY_PORT;

    while ((option = getopt(argc, argv, "s:w:T:l:c:d:p:")) != -1)
    {
        if (option == '?' || option == ':')
        {
            free(serverArgument);
            return NULL;
        }

        switch (option)
        {
        case 's':
            serverArgument->session = optarg;
            break;
        case 'w':
            serverArgument->window = atoi(optarg);
            break;
        case 'T':
            serverArgument->timeout_ms = atoi(optarg);
            break;
        case 'l':
            serverArgument->loss = strtod(optarg, NULL);
            break;
        case 'c':
            serverArgument->corrupt = strtod(optarg, NULL);
            break;
        case 'd':
            serverArgument->dup = strtod(optarg, NULL);
            break;
        case 'p':
            serverArgument->port = atoi(optarg);
            break;
        default:
            free(serverArgument);
            return NULL;
        }
    }

    if (serverArgument->session == NULL || argc - optind != 2)
    {
        free(serverArgument);
        return NULL;
    }

    serverArgument->relay = argv[optind];
    serverArgument->file_name = argv[optind + 1];

    return serverArgument;
}

int init_server(SERVER_ARGUMENT *server)
{
    int sock_fd = -1;
    struct addrinfo resolver;
    struct addrinfo *result = NULL;
    struct addrinfo *rp = NULL;

    memset(&resolver, 0, sizeof(resolver));
    resolver.ai_family = AF_UNSPEC;
    resolver.ai_socktype = SOCK_DGRAM;
    resolver.ai_protocol = IPPROTO_UDP;

    char port_string[INET6_ADDRSTRLEN];
    snprintf(port_string, sizeof(port_string), "%d", server->port);
    int status = getaddrinfo(server->relay, port_string, &resolver, &result);

    if (status != 0)
    {
        fprintf(stderr, "Error: Failed to look up the provided relay address");
        return 1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sock_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock_fd == -1)
        {
            continue;
        }

        if (connect(sock_fd, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break;
        }

        close(sock_fd);
        sock_fd = -1;
    }

    freeaddrinfo(result);
    return sock_fd;
}

int register_server(int fd, SERVER_ARGUMENT *server)
{
    char init_message[250];
    char reply[250];
    struct timeval receive_timeout = {
        .tv_sec = 1,
        .tv_usec = 0};

    if (session_validator(server->session) == 0)
    {
        return -1;
    }
    snprintf(init_message, sizeof(init_message), "HELLO %s send %.3g %.3g %.3g",
             server->session, server->loss, server->corrupt, server->dup);

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                   &receive_timeout, sizeof(receive_timeout)) < 0)
    {
        perror("Set receive timeout failed");
        return -1;
    }

    for (int attempt = 0; attempt < REGISTER_MAX_ATTEMPT; ++attempt)
    {
        if (send(fd, init_message, strlen(init_message), 0) < 0)
        {
            perror("Send register message failed");
            continue;
        }

        ssize_t reply_length = recv(fd, reply, sizeof(reply) - 1, 0);
        if (reply_length >= 0)
        {
            reply[reply_length] = '\0';
            registration_status register_status = evaluate_registration_response(reply, (size_t)reply_length);

            if (register_status == REG_FAILURE || register_status == REG_MALFORMED)
            {
                printf("Server failed with the reply: %s\n", reply);
                return 2;
            }
            printf("Server received reply: %s\n", reply);
            return 0;
        }

        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            perror("Receive register response failed");
        }
    }
    fprintf(stderr, "Relay registration exhaust all %d", REGISTER_MAX_ATTEMPT);
    return -1;
}

int publish(int fd, SERVER_ARGUMENT *server)
{
    int returnCode = 2;
    FILE_METADATA *fileMetadata = read_file(server->file_name);

    if (fileMetadata == NULL)
    {

        return returnCode;
    }
    packet_header packets[WINDOW_MAX];

    server_state *current_state = malloc(sizeof(*current_state));
    if (current_state == NULL)
    {
        perror("Not able to create sever state");
        free(fileMetadata->data);
        free(fileMetadata);
        return returnCode;
    }
    if ((unsigned)server->window > WINDOW_MAX || server->window < 1 || server->timeout_ms == 0)
    {
        free(fileMetadata->data);
        free(fileMetadata);
        return returnCode;
    }
    memset(current_state, 0, sizeof(*current_state));

    current_state->data = fileMetadata->data;
    current_state->size = fileMetadata->size;
    current_state->window = (unsigned)server->window;
    current_state->timeout_ms = (unsigned)server->timeout_ms;
    // Standard Division Rounds Down trick
    current_state->total_chunks = (uint32_t)((fileMetadata->size + PAYLOAD_SIZE - 1) /
                                             PAYLOAD_SIZE);
    int64_t now = get_time_ms();
    if (now < 0 || flush(fd, packets, populate_transmission_window(current_state, now, packets)) != 0)
    {
        free(current_state);
        free(fileMetadata->data);
        free(fileMetadata);
        return returnCode;
    }
    while (!current_state->finished && !current_state->failed)
    {
        now = get_time_ms();
        int64_t remaining_time_ms = current_state->limit_ms - now;
        if (remaining_time_ms <= 0)
        {
            size_t count = handle_retransmission_timeout(current_state, now, packets);
            if (current_state->failed)
            {
                fprintf(stderr, "Transfer short circuit after 10 retransmission.\n");
                break;
            }
            if (flush(fd, packets, count) != 0)
            {
                break;
            }
            continue;
        }
        struct pollfd pfd = {.fd = fd, .events = POLLIN};
        int remaining_timeout = poll(&pfd, 1, remaining_time_ms > INT_MAX ? INT_MAX : (int)remaining_time_ms);
        if (remaining_timeout < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("Idle mode waiting for client ACK");
            break;
        }
        if (remaining_timeout == 0)
        {
            continue;
        }
        uint8_t payloadBuffer[PAYLOAD_SIZE + PAYLOAD_SIZE];
        ssize_t receivedBytes = recv(fd, payloadBuffer, sizeof(payloadBuffer), 0);
        if (receivedBytes < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("Error in receiving data");
            break;
        }
        packet_header incoming_header;
        int parse_packet = parse_incoming_packet(payloadBuffer, (size_t)receivedBytes, &incoming_header);
        if (parse_packet != 0)
        {
            continue;
        }
        if (incoming_header.pack_type == ACK)
        {
            printf("Received ACK: seq=%u\n", (unsigned)incoming_header.seq_num);
        }
        now = get_time_ms();
        int handledValidAck = handle_valid_ack(current_state, &incoming_header, now);
        int handleFlush = flush(fd, packets, populate_transmission_window(current_state, now, packets));
        if (handledValidAck && !current_state->finished && handleFlush != 0)
        {
            break;
        }
    }
    if (current_state->finished)
    {
        returnCode = 0;
    }
    free(fileMetadata->data);
    free(fileMetadata);
    free(current_state);

    return returnCode;
}

/**
 * Populates the sliding window with fresh DATA packets up to the window capacity,
 * or generates a FIN packet if all payload data has been fully acknowledged.
 *
 * @param[in,out] state  Pointer to the sender's active state machine tracking object.
 * @param[in]     now_ms The current system timestamp in milliseconds (from a monotonic clock).
 * @param[out]    out    An allocated array where generated packets will be staged for transmission.
 * @return               The total number of packets successfully placed into the 'out' array.
 */
size_t populate_transmission_window(server_state *state, int64_t now_ms, packet_header *out)
{
    // 1. Guard clauses for invalid states or completed operations
    if (state == NULL || out == NULL || state->failed || state->finished)
    {
        return 0;
    }

    size_t count = 0;

    // 2. Loop to fill the sliding window with new DATA packets
    while (state->next < state->total_chunks &&
           (state->next - state->base) < state->window)
    {
        uint32_t seq = state->next;
        size_t slot_idx = seq % WINDOW_MAX;
        packet_header *packet = &state->standby[slot_idx];

        // Format packet metadata
        packet->pack_type = DATA;
        packet->seq_num = seq;

        // Calculate slice size and copy from raw data buffer
        size_t offset = (size_t)seq * PAYLOAD_SIZE;
        size_t remaining = state->size - offset;

        packet->data_len = PAYLOAD_SIZE;
        if (remaining < PAYLOAD_SIZE)
        {
            packet->data_len = (uint16_t)remaining;
        }

        memcpy(packet->data, state->data + offset, packet->data_len);

        // Stage the packet for output transmission
        out[count] = *packet;
        count++;

        // Start the retransmission timer if this is the oldest unacknowledged packet
        if (state->base == state->next)
        {
            state->limit_ms = now_ms + state->timeout_ms;
        }

        state->next++;
    }

    // 3. Emit a FIN packet once all data is fully sent AND acknowledged
    if (state->base == state->total_chunks && state->next == state->total_chunks)
    {
        size_t slot_idx = state->next % WINDOW_MAX;
        packet_header *packet = &state->standby[slot_idx];

        packet->pack_type = FIN;
        packet->seq_num = state->next;
        packet->data_len = 0;

        // Stage the FIN packet for transmission
        out[count] = *packet;
        count++;

        // Arm the timeout timer to monitor the FIN packet delivery
        state->limit_ms = now_ms + state->timeout_ms;
        state->next++;
    }

    return count;
}

int flush(int fd, const packet_header *packets, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        if (send_packet(fd, &packets[i]) != 0)
        {
            return -1;
        }
        const char *packet_type = packets[i].pack_type == DATA ? "DATA" : packets[i].pack_type == FIN ? "FIN"
                                                                                                      : "UNKNOWN";
        printf("Sent %s packet: seq=%u, payload=%zu bytes\n",
               packet_type, (unsigned)packets[i].seq_num, packets[i].data_len);
    }
    return 0;
}

/**
 * Manages packet retransmissions when the sliding window timer expires.
 * Copies all unacknowledged packets to the output buffer and permanently
 * marks the sender as failed if 10 consecutive attempts pass without progress.
 *
 * @param[in,out] state  Pointer to the sender's active state machine tracking object.
 * @param[in]     now_ms The current system timestamp in milliseconds (from a monotonic clock).
 * @param[out]    out    An allocated array where unacknowledged packets will be staged for retransmission.
 * @return               The total number of packets placed into the 'out' array for resending.
 */
size_t handle_retransmission_timeout(server_state *state, int64_t now_ms, packet_header *out)
{
    // 1. Guard clauses to ensure a timeout actually occurred and data is in flight
    if (state == NULL || out == NULL || state->failed || state->finished)
    {
        return 0;
    }

    if (state->base == state->next || now_ms < state->limit_ms)
    {
        return 0;
    }

    state->num_timeouts++;
    if (state->num_timeouts >= 10)
    {
        state->failed = 1;
        return 0;
    }

    size_t count = 0;

    for (uint32_t seq = state->base; seq < state->next; seq++)
    {
        size_t slot_idx = seq % WINDOW_MAX;
        out[count] = state->standby[slot_idx];
        count++;
    }

    state->limit_ms = now_ms + state->timeout_ms;

    return count;
}

/**
 * Evaluates an incoming acknowledgment packet. On cumulative progress, slides
 * the transmission window base, resets the network failure counter, and updates
 * the retransmission deadline.
 *
 * @param[in,out] state  Pointer to the sender's active state machine tracking object.
 * @param[in]     ack    Pointer to the read-only packet data structure containing the ACK metadata.
 * @param[in]     now_ms The current system timestamp in milliseconds (from a monotonic clock).
 * @return               Returns 1 if the ACK was valid and advanced the window state; 0 otherwise.
 */
int handle_valid_ack(server_state *state, const packet_header *header, int64_t now_ms)
{
    if (state == NULL || header == NULL || state->failed || state->finished)
    {
        return 0;
    }

    if (header->pack_type != ACK || header->data_len != 0)
    {
        return 0;
    }

    if (header->seq_num <= state->base || header->seq_num > state->next)
    {
        return 0;
    }

    state->base = header->seq_num;
    state->num_timeouts = 0; // Connection is active; reset the failure safety circuit

    uint32_t final_fin_ack_target = state->total_chunks + 1;

    if (state->base == final_fin_ack_target)
    {
        state->finished = 1;
        state->limit_ms = 0;
    }
    else if (state->base == state->next)
    {
        state->limit_ms = 0;
    }
    else
    {
        state->limit_ms = now_ms + state->timeout_ms;
    }

    return 1;
}