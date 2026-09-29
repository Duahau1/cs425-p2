
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>
#include "lab.h"

#define STREQU(a, b) (strcmp(a, b) == 0)

uint16_t compute_checksum(const uint8_t *data, size_t length);

int session_validator(const char *session);

int send_packet(int sock_fd, const packet_header *incoming_packet);
int parse_incoming_packet(const uint8_t *packetPayload, size_t size, packet_header *outputHeader);

int64_t get_time_ms(void);

typedef struct
{
    uint8_t *data;
    size_t size;
} FILE_METADATA;

FILE_METADATA *read_file(const char *path);

#endif // UTILS_H
