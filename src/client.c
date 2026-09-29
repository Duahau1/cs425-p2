#include "lab.h"
#include "utils.h"
#include <errno.h>
#include <netdb.h>

#ifdef TEST
extern ssize_t test_send(int socket_fd, const void *buffer, size_t length, int flags);
extern ssize_t test_recv(int socket_fd, void *buffer, size_t length, int flags);
#define send test_send
#define recv test_recv
#endif

CLIENT_ARGUMENT *parse_cl_opt(int argc, char *const argv[])
{

    int option;
    char *session = NULL;
    char *relay = NULL;
    char *file_name = NULL;
    int port = RELAY_PORT;

    CLIENT_ARGUMENT *clientArgument = malloc(sizeof(*clientArgument));

    if (argc < 1 || argv == NULL || clientArgument == NULL)
    {
        free(clientArgument);
        return NULL;
    }

    while ((option = getopt(argc, argv, "s:p:")) != -1)
    {
        if (option == '?' || option == ':')
        {
            free(clientArgument);
            return NULL;
        }
        switch (option)
        {
        case 's':
            session = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;

        default:
            free(clientArgument);
            return NULL;
        }
    }
    int remaining_args = argc - optind;

    if (remaining_args != 2)
    {
        fprintf(stderr, "Error: Expected exactly 2 positional arguments (<relay> and <file>), but got %d.\n", remaining_args);
        fprintf(stderr, "Usage: %s -s <session> [-p port] <relay> <file>\n", argv[0]);
        free(clientArgument);
        return NULL;
    }

    relay = argv[optind];
    file_name = argv[optind + 1];

    clientArgument->session = session;
    clientArgument->port = port;
    clientArgument->relay = relay;
    clientArgument->file_name = file_name;

    return clientArgument;
}

int init_client(CLIENT_ARGUMENT *client)
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
    snprintf(port_string, sizeof(port_string), "%d", client->port);
    int status = getaddrinfo(client->relay, port_string, &resolver, &result);

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

int register_client(int socket_fd, CLIENT_ARGUMENT *client)
{
    char register_message[250];
    char reply[250];

    struct timeval receive_timeout = {
        .tv_sec = 1,
        .tv_usec = 0};

    if (session_validator(client->session) == 0)
    {
        return -1;
    }
    snprintf(register_message, sizeof(register_message), "HELLO %s recv", client->session);

    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO,
                   &receive_timeout, sizeof(receive_timeout)) < 0)
    {
        perror("Set receive timeout failed");
        return -1;
    }

    for (int attempt = 0; attempt < REGISTER_MAX_ATTEMPT; ++attempt)
    {
        if (send(socket_fd, register_message, strlen(register_message), 0) < 0)
        {
            perror("Send register message failed");
            continue;
        }

        ssize_t reply_length = recv(socket_fd, reply, sizeof(reply) - 1, 0);
        if (reply_length >= 0)
        {
            // TODO: error error
            reply[reply_length] = '\0';
            printf("Client received reply: %s, with reply_length: %zd\n", reply, reply_length);
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

int process(int fd, CLIENT_ARGUMENT *client)
{
    int returnCode = 2;
    FILE *file = fopen(client->file_name, "wb");
    if (file == NULL)
    {
        perror("Unable to process file");
        return returnCode;
    }
    client_state *current_state = malloc(sizeof(*current_state));
    if (current_state == NULL)
    {
        perror("Not able to create sever state");
        free(current_state);
        fclose(file);
        return returnCode;
    }
    memset(current_state, 0, sizeof(*current_state));
    int64_t now = get_time_ms();

    current_state->last_valid_ms = now;
    while (1)
    {
        now = get_time_ms();
        if (current_state->finished && now >= current_state->linger_time_ms)
        {
            returnCode = 0;
            break;
        }
        else
        {
            if (now - current_state->last_valid_ms >= 30000)
            {

                fprintf(stderr, "Receiver timed outputHeader after 30 seconds idle.\n");
                break;
            }
        }
        struct pollfd pfd = {.fd = fd, .events = POLLIN};
        int remaining_timeout = get_remaining_timeout_ms(current_state, now);
        if (remaining_timeout < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("Idle mode waiting for data");
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
        int parse_packet = parse_incoming_packet(payloadBuffer, receivedBytes, &incoming_header);
        if (parse_packet != 0)
        {
            continue;
        }
        consume(fd, current_state, &incoming_header, file);
    }
    fclose(file);
    free(current_state);
    return returnCode;
}

int get_remaining_timeout_ms(const client_state *state, int64_t now)
{
    if (state == NULL)
    {
        return 0;
    }

    int64_t deadline = state->linger_time_ms;
    if (!state->finished)
    {
        const int64_t INACTIVITY_TIMEOUT_MS = 30000;
        deadline = state->last_valid_ms + INACTIVITY_TIMEOUT_MS;
    }

    int64_t remaining = deadline - now;
    if (remaining <= 0)
    {
        return 0;
    }

    // Safely clamp to INT_MAX to prevent 32-bit truncation errors during type casting
    if (remaining > INT_MAX)
    {
        return INT_MAX;
    }

    return (int)remaining;
}

int consume(int sock_fd, client_state *client_state, packet_header *incoming_packet, FILE *opened_file)
{
    client_state->last_valid_ms = get_time_ms();

    // Case 1: In-order DATA packet
    if (client_state->finished == 0 && incoming_packet->pack_type == DATA && incoming_packet->seq_num == client_state->expected)
    {
        if (incoming_packet->data_len > 0)
        {
            fwrite(incoming_packet->data, 1, incoming_packet->data_len, opened_file);
        }
        client_state->expected++;
        send_ack(sock_fd, incoming_packet);
    }
    // Case 2: Out-of-order or duplicate DATA packet
    else if (incoming_packet->pack_type == DATA)
    {
        send_ack(sock_fd, incoming_packet);
    }
    // Case 3: In-order FIN packet
    else if (incoming_packet->pack_type == FIN && incoming_packet->seq_num == client_state->expected)
    {
        fclose(opened_file);
        client_state->expected++;
        send_ack(sock_fd, incoming_packet);

        // Initiate the 2-second linger phase
        client_state->finished = 1;
        client_state->linger_time_ms = get_time_ms() + 2000;
    }
    // Case 4: Repeated FIN during the linger window
    else if (incoming_packet->pack_type == FIN && client_state->finished)
    {
        // Answer repeated FINs with the exact same ACK to help the sender close cleanly
        send_ack(sock_fd, incoming_packet);
    }
    return 0;
}
int send_ack(int sock_fd, packet_header *incoming_packet)
{
    // ACK packets contain only the 10-byte header (zero data payload length)
    uint8_t ack_packet[MAX_SIZE_OF_FILE];
    size_t totalBytes = HEADER_SIZE + incoming_packet->data_len;

    // Clear out the memory layout entirely (handles padding/reserved fields)
    memset(ack_packet, 0, sizeof(ack_packet));

    ack_packet[0] = ACK;

    ack_packet[4] = (uint8_t)(incoming_packet->seq_num >> 24);
    ack_packet[5] = (uint8_t)(incoming_packet->seq_num >> 16);
    ack_packet[6] = (uint8_t)(incoming_packet->seq_num >> 8);
    ack_packet[7] = (uint8_t)incoming_packet->seq_num;

    ack_packet[8] = (uint8_t)(incoming_packet->data_len >> 8);
    ack_packet[9] = (uint8_t)incoming_packet->data_len;

    // 5. Compute the RFC 1071 Checksum with the checksum region initially zeroed out
    // The compute_checksum function automatically takes the 1s complement.
    uint16_t checksum = compute_checksum(ack_packet, totalBytes);
    ack_packet[2] = (uint8_t)(checksum >> 8);
    ack_packet[3] = (uint8_t)checksum;
    ssize_t bytes_sent = send(sock_fd, ack_packet, totalBytes, 0);
    if (bytes_sent < 0)
    {
        perror("Failed to send ACK packet");
        return -1;
    }
    return 0;
}
