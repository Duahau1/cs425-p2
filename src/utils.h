
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

#define STREQU(a, b) (strcmp(a, b) == 0)

uint16_t compute_checksum(const uint8_t *data, size_t length);

int is_relay_addr_valid(char *relay, int port);

int session_validator(const char *session);

#endif // UTILS_H
