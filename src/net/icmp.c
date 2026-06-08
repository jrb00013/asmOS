#include "net.h"
#include "platform.h"
#include <stdint.h>

#define ICMP_ECHO 8
#define ICMP_ECHOREPLY 0

typedef struct {
    uint8_t ver_ihl;
    uint8_t tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t frag;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src;
    uint32_t dst;
} ip_hdr_t;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} icmp_hdr_t;

static uint16_t checksum(const void *data, int len) {
    const uint16_t *p = (const uint16_t *)data;
    uint32_t sum = 0;
    while (len > 1) { sum += *p++; len -= 2; }
    if (len) sum += *(const uint8_t *)p;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)(~sum);
}

int net_icmp_ping(uint32_t dst_ip, uint32_t *rtt_ms) {
    uint8_t pkt[64];
    ip_hdr_t *ip = (ip_hdr_t *)pkt;
    icmp_hdr_t *icmp = (icmp_hdr_t *)(pkt + 20);
    plat_net_info_t info;
    plat_net_get_info(&info);

    ip->ver_ihl = 0x45;
    ip->ttl = 64;
    ip->protocol = 1;
    ip->src = info.ip;
    ip->dst = dst_ip;
    ip->total_len = 28;
    ip->checksum = 0;
    ip->checksum = checksum(ip, 20);

    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->id = 0x4D4F;
    icmp->seq = 1;
    icmp->checksum = 0;
    icmp->checksum = checksum(icmp, 8);

    uint32_t start = plat_ticks_ms();
    plat_net_send(pkt, 28);

    uint8_t rx[128];
    int tries = 2000;
    while (tries-- > 0) {
        int n = plat_net_recv(rx, sizeof(rx));
        if (n >= 28) {
            icmp_hdr_t *ricmp = (icmp_hdr_t *)(rx + 20);
            if (ricmp->type == ICMP_ECHOREPLY) {
                if (rtt_ms) *rtt_ms = plat_ticks_ms() - start;
                return 0;
            }
        }
        plat_delay_ms(1);
    }
    return -1;
}
