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
#include <errno.h>
#include <poll.h>
#include "lab.h"

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

FILE_METADATA *read_file(const char *path)
{
    FILE *inputFile;
    FILE_METADATA *file = NULL;

    if ((inputFile = fopen(path, "rb")) == NULL)
    {
        perror("fopen");
        return NULL;
    }

    fseek(inputFile, 0, SEEK_END);

    long file_size = ftell(inputFile);
    if (file_size < 0 || file_size > MAX_SIZE_OF_FILE)
    {
        perror("Error getting file position");
        fclose(inputFile);
        return NULL;
    }

    fseek(inputFile, 0, SEEK_SET);

    file = malloc(sizeof(*file));
    if (file == NULL)
    {
        perror("Memory allocation failed");
        fclose(inputFile);
        return NULL;
    }

    file->data = malloc((size_t)file_size);
    if (file->data == NULL && file_size != 0)
    {
        perror("Memory allocation failed");
        free(file);
        fclose(inputFile);
        return NULL;
    }

    size_t bytes_read = fread(file->data, 1, (size_t)file_size, inputFile);

    if (bytes_read < (size_t)file_size)
    {
        if (ferror(inputFile))
        {
            perror("Error reading file");
        }
        else if (feof(inputFile))
        {
            printf("Warning: Hit End-of-File early. Read %zu of %ld bytes.\n", bytes_read, file_size);
        }
    }

    fclose(inputFile);

    file->size = bytes_read;
    return file;
}

/**
 * Returns the current system time in milliseconds using a monotonic clock.
 * Guaranteed never to move backwards, making it ideal for timeouts and intervals.
 *
 * @return Current timestamp in milliseconds, or -1 on error.
 */
int64_t get_time_ms(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        return -1;
    }

    const int64_t MS_PER_SEC = 1000;
    const int64_t NS_PER_MS = 1000000;

    return ((int64_t)ts.tv_sec * MS_PER_SEC) + (ts.tv_nsec / NS_PER_MS);
}