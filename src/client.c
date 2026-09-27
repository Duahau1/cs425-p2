#include "lab.h"
#include "utils.h"
#include <netdb.h>

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
int register_client(int fd, CLIENT_ARGUMENT *client)
{
    char init_message[250];

    if (session_validator(client->session) == 0)
    {
        return -1;
    }
    snprintf(init_message, sizeof(init_message), "HELLO %s recv", client->session);
}
