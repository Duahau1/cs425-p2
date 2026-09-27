#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <regex.h>

uint16_t compute_checksum(const uint8_t *data, size_t length)
{
    uint32_t sum = 0;

    while (length > 1)
    {
        sum += ((uint32_t)data[0] << 8) | data[1];
        data += 2;
        length -= 2;
    }

    if (length == 1)
    {
        sum += ((uint32_t)data[0] << 8);
    }

    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}

int is_relay_addr_valid(char *relay, int port)
{
    return 0;
}

// Returns 1 when the session name is valid; otherwise returns 0.
int session_validator(const char *session_name)
{
    regex_t pattern;
    int result;

    if (session_name == NULL)
    {
        return 0;
    }

    result = regcomp(&pattern, "^[a-z0-9-]{1,32}$", REG_EXTENDED | REG_NOSUB);
    if (result != 0)
    {
        return 0;
    }

    result = regexec(&pattern, session_name, 0, NULL, 0) == 0;
    regfree(&pattern);

    return result;
}
