#include "lab.h"
#include "utils.h"

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
            serverArgument->loss = atoi(optarg);
            break;
        case 'c':
            serverArgument->corrupt = atoi(optarg);
            break;
        case 'd':
            serverArgument->dup = atoi(optarg);
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