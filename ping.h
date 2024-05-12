#ifndef PING_H

#include <stdint.h>
#include <netinet/ip_icmp.h>

uint16_t checksum(uint16_t *buf, int32_t len);

struct icmp new_icmp_echo_header(
    uint8_t type,
    uint8_t code,
    uint16_t identifier,
    uint16_t sequence_number,
    uint32_t data
);
#endif
