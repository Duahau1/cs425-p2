#include "utils.h"
#include "lab.h"

#ifdef TEST
#define main main_exclude
#endif

void print_manual(void)
{
    puts("Usage: myapp send -s <session> [-w window] [-T timeout-ms] [-l loss]");
    puts("                  [-c corrupt] [-d dup] [-p port] <relay> <file>");
    puts("       myapp recv -s <session> [-p port] <relay> <file>");
    puts("");
    puts("  -s <session>     session name shared by the sender and the receiver");
    puts("  -w <window>      Go-Back-N window size in packets, 1 to 64 (default: 8)");
    puts("  -T <timeout-ms>  retransmission timeout in milliseconds (default: 250)");
    puts("  -l <loss>        probability the relay drops a packet (default: 0)");
    puts("  -c <corrupt>     probability the relay flips a bit (default: 0)");
    puts("  -d <dup>         probability the relay duplicates a packet (default: 0)");
    puts("  -p <port>        relay port (default: 4250)");
    puts("  <relay>          host name or address of the relay");
    puts("  <file>           file to send, or file to write what is received");
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        print_manual();
        return 0;
    }

    char *parser_argv[argc - 1];
    parser_argv[0] = argv[0];
    for (int index = 1; index < argc - 1; index++)
    {
        parser_argv[index] = argv[index + 1];
    }

    if (strcmp(argv[1], "send") == 0)
    {
        SERVER_ARGUMENT *serverArgument = parse_ser_opt(argc - 1, parser_argv);

        // printf("send: session=%s, window=%d, timeout=%d, loss=%d, corrupt=%d, dup=%d, port=%d, relay=%s, file=%s\n",
        //        serverArgument->session,
        //        serverArgument->window,
        //        serverArgument->timeout,
        //        serverArgument->loss,
        //        serverArgument->corrupt,
        //        serverArgument->dup,
        //        serverArgument->port,
        //        serverArgument->relay,
        //        serverArgument->file_name);
        if (serverArgument == NULL || serverArgument->session == NULL ||
            serverArgument->relay == NULL || serverArgument->file_name == NULL)
        {
            print_manual();
            free(serverArgument);
            return 1;
        }
        free(serverArgument);
    }
    else if (strcmp(argv[1], "recv") == 0)
    {
        CLIENT_ARGUMENT *clientArgument = parse_cl_opt(argc - 1, parser_argv);
        // printf("recv: session=%s, port=%d, relay=%s, file=%s\n",
        //        clientArgument->session,
        //        clientArgument->port,
        //        clientArgument->relay,
        //        clientArgument->file_name);
        if (clientArgument == NULL || clientArgument->session == NULL ||
            clientArgument->relay == NULL || clientArgument->file_name == NULL)
        {
            print_manual();
            free(clientArgument);
            return 1;
        }
        free(clientArgument);
    }
    else
    {
        print_manual();
        return 0;
    }
}