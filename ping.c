#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>

#include "ping.h"

void usage_message(char *program_name)
{
    fprintf(stderr, "Usage: %s [-i] file_name destination\n", program_name);
    exit(1);
}

uint16_t checksum(uint16_t *buf, int32_t len)
{
    int32_t remaining = len;
    int32_t sum = 0;
    uint16_t *buf_ = buf;

    while(remaining > 1)
    {
        sum += *buf_++;
        remaining -= 2;
    }

    if (remaining == 1)
    {
        sum += *(uint8_t *)buf_;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return (uint16_t)~sum;
}

struct icmp new_icmp_echo_header(
    uint8_t type,
    uint8_t code,
    uint16_t identifier,
    uint16_t sequence_number,
    uint32_t data
)
{
    struct icmp packet;

    packet.icmp_type = type;
    packet.icmp_code = code;
    packet.icmp_cksum = 0;
    packet.icmp_id = htons(identifier);
    packet.icmp_seq = htons(sequence_number);
    packet.icmp_mask = data;

    packet.icmp_cksum = checksum((uint16_t *)&packet, sizeof(packet));

    return packet;
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    char * identifier_arg = NULL;
    char * file_name = NULL;
    char * destination = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "i:")) != -1)
    {
        switch (opt)
        {
            case 'i':
                identifier_arg = optarg;
                break;
            default:
                usage_message(argv[0]);
        }
    }

    if (optind + 1 < argc) {
        file_name = argv[optind];
        destination = argv[optind + 1];
    }

    if (destination == NULL || file_name == NULL)
    {
        usage_message(argv[0]);
    }

    int socket_result = socket(PF_INET, SOCK_RAW, 1);

    if (socket_result <= 0)
    {
        exit(2);
    }

    FILE * file_stream = fopen(file_name, "rb");
    if (file_stream == NULL) 
    {
        exit(3);
    }

    uint8_t data_in[4] = {0};
    size_t data_in_size = sizeof(*data_in);

    struct in_addr destination_ip;
    int inet_pton_result = inet_pton(AF_INET, destination, &destination_ip);

    if (inet_pton_result != 1)
    {
        exit(4);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = 0;
    address.sin_addr.s_addr = destination_ip.s_addr;

    uint16_t identifier;
    if (identifier_arg != NULL)
    {
        identifier = (uint16_t)atoi(identifier_arg);
    }
    else
    {
        identifier = (uint16_t)rand();
    }
    uint16_t sequence_number = 1;

    while (!feof(file_stream))
    {
        // ignoring the result of fread as we are checking for eof in loop condition
        // and don't care about errors.
        fread(data_in, data_in_size, 4, file_stream);

        uint32_t data = data_in[3] << 24 | data_in[2] << 16 | data_in[1] << 8 | data_in[0];

        struct icmp packet = new_icmp_echo_header(
            ICMP_ECHO,
            0,
            identifier,
            sequence_number,
            data
        );

        int sendto_result = sendto(
            socket_result,
            &packet,
            sizeof(packet),
            0,
            (struct sockaddr*)&address,
            sizeof(address)
        );

        if (sendto_result < 0)
        {
            fclose(file_stream);
            exit(5);
        }

        sequence_number++;
        memset(data_in, 0, 4);

    }

    fclose(file_stream);
    return 0;
}
