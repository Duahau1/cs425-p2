
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>
#define MAX_SIZE_OF_FILE (16U * 1024U * 1024U)

#define STREQU(a, b) (strcmp(a, b) == 0)

uint16_t compute_checksum(const uint8_t *data, size_t length);

int is_relay_addr_valid(char *relay, int port);

int session_validator(const char *session);

int64_t get_time_ms(void);

typedef struct
{
    uint8_t *data;
    size_t size;
} FILE_METADATA;

FILE_METADATA *read_file(const char *path);

#endif // UTILS_H
