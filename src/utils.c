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
/**
 * Send the package
 */
int send_packet(int sock_fd, const packet_header *incoming_packet)
{
    uint8_t ack_packet[HEADER_SIZE];
    size_t totalBytes = HEADER_SIZE + incoming_packet->data_len;

    memset(ack_packet, 0, sizeof(ack_packet));

    ack_packet[0] = incoming_packet->pack_type;

    ack_packet[4] = (uint8_t)(incoming_packet->seq_num >> 24);
    ack_packet[5] = (uint8_t)(incoming_packet->seq_num >> 16);
    ack_packet[6] = (uint8_t)(incoming_packet->seq_num >> 8);
    ack_packet[7] = (uint8_t)incoming_packet->seq_num;

    ack_packet[8] = (uint8_t)(incoming_packet->data_len >> 8);
    ack_packet[9] = (uint8_t)incoming_packet->data_len;

    uint16_t checksum = compute_checksum(ack_packet, totalBytes);
    ack_packet[2] = (uint8_t)(checksum >> 8);
    ack_packet[3] = (uint8_t)checksum;
    ssize_t bytes_sent = send(sock_fd, ack_packet, totalBytes, 0);
    if (bytes_sent < 0)
    {
        perror("Failed to send packet");
        return -1;
    }
    return 0;
}
int parse_incoming_packet(const uint8_t *packetPayload, size_t size, packet_header *outputHeader)
{
    if (packetPayload == NULL || outputHeader == NULL || size < HEADER_SIZE)
    {
        return -1;
    }

    uint8_t type = packetPayload[0];
    uint8_t reserved = packetPayload[1];

    if (type > FIN || reserved != 0)
    {
        return -1;
    }

    uint16_t data_length = (uint16_t)(((uint16_t)packetPayload[8] << 8) | packetPayload[9]);

    if (data_length > PAYLOAD_SIZE)
    {
        return -1;
    }
    if (HEADER_SIZE + data_length != size)
    {
        return -1;
    }
    if (type != DATA && data_length != 0)
    {
        return -1; // Only DATA packets are allowed to carry a payload
    }

    if (compute_checksum(packetPayload, size) != 0)
    {
        return -1;
    }

    packet_header parsed_data = {
        .pack_type = type,
        .data_len = data_length,
        .seq_num = ((uint32_t)packetPayload[4] << 24) |
                   ((uint32_t)packetPayload[5] << 16) |
                   ((uint32_t)packetPayload[6] << 8) |
                   (uint32_t)packetPayload[7]};

    if (data_length > 0)
    {
        memcpy(parsed_data.data, packetPayload + HEADER_SIZE, data_length);
    }

    *outputHeader = parsed_data;
    return 0;
}