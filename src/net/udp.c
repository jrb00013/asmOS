/* Minimal UDP + ping for freestanding ASMOS. Uses plat_net for I/O. */

#include "net.h"
#include "platform.h"
#include "kernel.h"
#include <stdint.h>

#define RX_SLOTS 8
#define RX_SLOT_SIZE 1536

typedef struct {
    uint32_t src_ip;
    uint16_t src_port;
    uint16_t len;
    uint8_t data[NET_MAX_PAYLOAD];
} rx_slot_t;

static rx_slot_t rx_ring[RX_SLOTS];
static int rx_head, rx_tail;
static int net_up;

static uint32_t parse_octet(const char **p) {
    uint32_t v = 0;
    while (**p >= '0' && **p <= '9') {
        v = v * 10 + (uint32_t)(**p - '0');
        (*p)++;
    }
    return v;
}

int net_parse_ip(const char *str, uint32_t *out) {
    if (!str || !out) return -1;
    const char *p = str;
    uint32_t a = parse_octet(&p);
    if (*p != '.') return -1; p++;
    uint32_t b = parse_octet(&p);
    if (*p != '.') return -1; p++;
    uint32_t c = parse_octet(&p);
    if (*p != '.') return -1; p++;
    uint32_t d = parse_octet(&p);
    *out = (a << 24) | (b << 16) | (c << 8) | d;
    return 0;
}

void net_ip_to_str(uint32_t ip, char *buf, int max) {
    if (!buf || max < 8) return;
    char tmp[16];
    int pos = 0, seg, s;
    for (s = 3; s >= 0; s--) {
        uint32_t oct = (ip >> (s * 8)) & 0xFF;
        if (oct >= 100) tmp[pos++] = '0' + (char)(oct / 100);
        if (oct >= 10) tmp[pos++] = '0' + (char)((oct / 10) % 10);
        tmp[pos++] = '0' + (char)(oct % 10);
        if (s > 0) tmp[pos++] = '.';
    }
    tmp[pos] = '\0';
    int i = 0;
    while (tmp[i] && i < max - 1) { buf[i] = tmp[i]; i++; }
    buf[i] = '\0';
}

static void ring_push(uint32_t sip, uint16_t sport, const void *data, uint16_t len) {
    int next = (rx_tail + 1) % RX_SLOTS;
    if (next == rx_head) return;
    rx_ring[rx_tail].src_ip = sip;
    rx_ring[rx_tail].src_port = sport;
    rx_ring[rx_tail].len = len;
    uint16_t i;
    for (i = 0; i < len && i < NET_MAX_PAYLOAD; i++)
        rx_ring[rx_tail].data[i] = ((const uint8_t *)data)[i];
    rx_tail = next;
}

int net_init(void) {
    rx_head = rx_tail = 0;
    net_up = 1;
    return 0;
}

void net_shutdown(void) {
    net_up = 0;
}

int net_udp_send(uint32_t dst_ip, uint16_t dst_port, const void *data, uint16_t len) {
    if (!net_up) return -1;
    uint8_t pkt[32 + NET_MAX_PAYLOAD];
    uint16_t i = 0;
    pkt[i++] = 'A'; pkt[i++] = 'S'; pkt[i++] = 'M';
    pkt[i++] = (uint8_t)((dst_ip >> 24) & 0xFF);
    pkt[i++] = (uint8_t)((dst_ip >> 16) & 0xFF);
    pkt[i++] = (uint8_t)((dst_ip >> 8) & 0xFF);
    pkt[i++] = (uint8_t)(dst_ip & 0xFF);
    pkt[i++] = (uint8_t)(dst_port & 0xFF);
    pkt[i++] = (uint8_t)(dst_port >> 8);
    pkt[i++] = (uint8_t)(len & 0xFF);
    pkt[i++] = (uint8_t)(len >> 8);
    uint16_t j;
    for (j = 0; j < len && i < sizeof(pkt); j++)
        pkt[i++] = ((const uint8_t *)data)[j];
    plat_net_send(pkt, i);
    plat_net_info_t info;
    plat_net_get_info(&info);
    if (dst_ip == info.ip || dst_ip == 0x7F000001u)
        ring_push(info.ip, dst_port, data, len);
    return (int)len;
}

int net_udp_recv(uint32_t *src_ip, uint16_t *src_port, void *buf, uint16_t max_len) {
    if (!net_up) return -1;
    uint8_t raw[1536];
    int n = plat_net_recv(raw, sizeof(raw));
    if (n >= 11 && raw[0] == 'A' && raw[1] == 'S' && raw[2] == 'M') {
        uint32_t sip = ((uint32_t)raw[3] << 24) | ((uint32_t)raw[4] << 16) |
                       ((uint32_t)raw[5] << 8) | raw[6];
        uint16_t sport = raw[7] | ((uint16_t)raw[8] << 8);
        uint16_t plen = raw[9] | ((uint16_t)raw[10] << 8);
        if (11 + plen <= (uint16_t)n) {
            ring_push(sip, sport, raw + 11, plen);
        }
    }
    if (rx_head == rx_tail) return 0;
    rx_slot_t *s = &rx_ring[rx_head];
    uint16_t cpy = s->len;
    if (cpy > max_len) cpy = max_len;
    uint16_t i;
    for (i = 0; i < cpy; i++) ((uint8_t *)buf)[i] = s->data[i];
    if (src_ip) *src_ip = s->src_ip;
    if (src_port) *src_port = s->src_port;
    rx_head = (rx_head + 1) % RX_SLOTS;
    return (int)cpy;
}

int net_ping(const char *host, uint32_t *rtt_ms) {
    uint32_t ip;
    if (net_parse_ip(host, &ip) != 0) return -1;
    uint32_t start = plat_ticks_ms();
    const char ping[] = "PING";
    net_udp_send(ip, NET_PORT_PING, ping, 4);
    uint8_t buf[16];
    uint32_t sip;
    uint16_t sport;
    int tries = 1000;
    while (tries-- > 0) {
        int n = net_udp_recv(&sip, &sport, buf, sizeof(buf));
        if (n >= 4 && buf[0] == 'P') {
            if (rtt_ms) *rtt_ms = plat_ticks_ms() - start;
            return 0;
        }
        plat_delay_ms(1);
    }
    return -1;
}

struct net_stream {
    uint32_t host;
    uint16_t port;
    int open;
};

net_stream_t *net_connect(const char *host, uint16_t port) {
    net_stream_t *s = (net_stream_t *)0;
    static net_stream_t pool[2];
    int i;
    for (i = 0; i < 2; i++) {
        if (!pool[i].open) { s = &pool[i]; break; }
    }
    if (!s) return 0;
    if (net_parse_ip(host, &s->host) != 0) return 0;
    s->port = port;
    s->open = 1;
    return s;
}

int net_stream_send(net_stream_t *s, const void *data, size_t len) {
    if (!s || !s->open) return -1;
    return net_udp_send(s->host, s->port, data, (uint16_t)len);
}

int net_stream_recv(net_stream_t *s, void *buf, size_t max_len) {
    if (!s || !s->open) return -1;
    uint32_t sip;
    uint16_t sport;
    int n = net_udp_recv(&sip, &sport, buf, (uint16_t)max_len);
    (void)s;
    return n;
}

void net_stream_close(net_stream_t *s) {
    if (s) s->open = 0;
}
