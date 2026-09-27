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
            reply[reply_length] = '\0';
            printf("Server received reply: %s\n", reply);
            return 0;
        }

        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            perror("Receive register response failed");
        }
    }

    return -1;
}
