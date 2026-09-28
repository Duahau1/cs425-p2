#include "lab.h"
#include "utils.h"
#include <errno.h>

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
    serverArgument->timeout = TIMEOUT;
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
            serverArgument->timeout = atoi(optarg);
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
            // TODO: error error
            reply[reply_length] = '\0';
            printf("Server received reply: %s\n", reply);
            return 0;
        }

        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            perror("Receive register response failed");
        }
    }
    fprintf(stderr, "Relay registration exhaust all %s", REGISTER_MAX_ATTEMPT);
    return -1;
}

int publish(int fd, SERVER_ARGUMENT *server)
{
    FILE_METADATA *fileMetadata = read_file(server->file_name);

    if (fileMetadata == NULL)
    {

        return 1;
    }
    packet_header packets[WINDOW_MAX];

    server_state *server_state = malloc(sizeof(*server_state));
    if (server_state == NULL)
    {
        perror("Not able to create sever state");
        free(fileMetadata);
        return 2;
    }
    if (server->window > WINDOW_MAX || server->window < 1 || server->timeout == 0)
    {
        free(fileMetadata);
        return 1;
    }
    memset(server_state, 0, sizeof(*server_state));

    server_state->data = fileMetadata->data;
    server_state->size = fileMetadata->size;
    server_state->window = server->window;
    server_state->timeout = server->timeout;
    // Standard Division Rounds Down trick
    server_state->total_chunks = (uint32_t)((fileMetadata->size + PAYLOAD_SIZE - 1) /
                                            PAYLOAD_SIZE);
    int64_t now = get_time_ms();
    if (now < 0 || flush(fd, packets, populate_transmission_window(server_state, now, packets)) != 0)
    {
        free(server_state);
        free(fileMetadata);
        return 1;
    }

    free(fileMetadata);
    free(server_state);

    return 0;
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
            state->limit_ms = now_ms + state->timeout;
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
        state->limit_ms = now_ms + state->timeout;
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
    }
    return 0;
}