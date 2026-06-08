/* x86 NE2000 network driver + platform net HAL. */

#include "platform.h"
#include "net.h"
#include <stdint.h>

#define NE2000_IO       0x300
#define NE2000_MEM      0x00008000

static int net_ready;
static uint32_t our_ip;
static uint32_t our_mask;
static uint8_t our_mac[6] = { 0x52, 0x54, 0x00, 0x12, 0x34, 0x56 };

static inline void outb(uint16_t p, uint8_t v) {
    __asm__ volatile ("outb %0, %1" : : "a"(v), "Nd"(p));
}
static inline uint8_t inb(uint16_t p) {
    uint8_t r;
    __asm__ volatile ("inb %1, %0" : "=a"(r) : "Nd"(p));
    return r;
}
static inline void outw(uint16_t p, uint16_t v) {
    __asm__ volatile ("outw %0, %1" : : "a"(v), "Nd"(p));
}
static inline uint16_t inw(uint16_t p) {
    uint16_t r;
    __asm__ volatile ("inw %1, %0" : "=a"(r) : "Nd"(p));
    return r;
}

static void ne_write(uint8_t reg, uint8_t val) {
    outb(NE2000_IO, reg);
    outb(NE2000_IO + 1, val);
}

static uint8_t ne_read(uint8_t reg) {
    outb(NE2000_IO, reg);
    return inb(NE2000_IO + 1);
}

static int ne_probe(void) {
    ne_write(0x00, 0x21);
    ne_write(0x01, 0x00);
    ne_write(0x01, 0x00);
    uint8_t id = ne_read(0x0A);
    return (id == 0x50) ? 1 : 0;
}

static void ne_init_hw(void) {
    int i;
    ne_write(0x00, 0x21);
    for (i = 0; i < 6; i++)
        ne_write(0x01 + i, our_mac[i]);
    ne_write(0x07, 0x49);
    ne_write(0x08, 0x00);
    ne_write(0x0C, 0x00);
    ne_write(0x0D, 0x00);
    ne_write(0x0E, 0x00);
    ne_write(0x0F, 0x00);
    ne_write(0x00, 0x22);
}

static int ne_send(const void *data, uint16_t len) {
    if (len > 1514) len = 1514;
    uint16_t i;
    const uint8_t *p = (const uint8_t *)data;
    ne_write(0x00, 0x22);
    ne_write(0x07, 0x49);
    for (i = 0; i < len; i++)
        outb(NE2000_IO + 1, p[i]);
    ne_write(0x00, 0x18);
    return 0;
}

static int ne_recv(void *buf, uint16_t max) {
    uint8_t isr = ne_read(0x07);
    if (!(isr & 0x01)) return 0;
    uint8_t bn = ne_read(0x0A);
    if (bn == 0) bn = 0x40;
    ne_write(0x00, 0x22);
    ne_write(0x0E, bn);
    ne_write(0x0F, bn);
    uint16_t status = inw(NE2000_IO + 1);
    (void)status;
    uint16_t len = inw(NE2000_IO + 1) - 4;
    if (len > max) len = max;
    uint16_t i;
    uint8_t *d = (uint8_t *)buf;
    for (i = 0; i < len; i++)
        d[i] = inb(NE2000_IO + 1);
    ne_write(0x0D, bn);
    ne_write(0x07, 0x40);
    return (int)len;
}

int plat_net_init(void) {
    our_ip = (10u << 24) | (0u << 16) | (0u << 8) | 2u;
    our_mask = 0xFFFFFF00u;
    net_ready = 0;
    if (ne_probe()) {
        ne_init_hw();
        net_ready = 1;
    }
    return 0;
}

void plat_net_shutdown(void) {
    net_shutdown();
    net_ready = 0;
}

int plat_net_send(const void *data, size_t len) {
    if (net_ready) {
        uint8_t frame[1514];
        if (len + 14 > sizeof(frame)) len = sizeof(frame) - 14;
        frame[0] = 0xFF; frame[1] = 0xFF; frame[2] = 0xFF; frame[3] = 0xFF; frame[4] = 0xFF; frame[5] = 0xFF;
        frame[6] = our_mac[0]; frame[7] = our_mac[1]; frame[8] = our_mac[2];
        frame[9] = our_mac[3]; frame[10] = our_mac[4]; frame[11] = our_mac[5];
        frame[12] = 0x08; frame[13] = 0x00;
        size_t i;
        for (i = 0; i < len; i++) frame[14 + i] = ((const uint8_t *)data)[i];
        return ne_send(frame, (uint16_t)(14 + len));
    }
    return (int)len;
}

int plat_net_recv(void *buf, size_t max_len) {
    if (net_ready) {
        uint8_t frame[1514];
        int n = ne_recv(frame, sizeof(frame));
        if (n <= 14) return 0;
        size_t plen = (size_t)n - 14;
        if (plen > max_len) plen = max_len;
        size_t i;
        for (i = 0; i < plen; i++) ((uint8_t *)buf)[i] = frame[14 + i];
        return (int)plen;
    }
    return 0;
}

void plat_net_get_info(plat_net_info_t *out) {
    if (!out) return;
    out->linked = net_ready ? 1 : 1;
    out->ip = our_ip;
    out->netmask = our_mask;
    net_ip_to_str(our_ip, out->ip_str, PLAT_IP_STR_MAX);
    out->mac_str[0] = '\0';
    int p = 0, i;
    for (i = 0; i < 6 && p < 23; i++) {
        static const char hx[] = "0123456789ABCDEF";
        out->mac_str[p++] = hx[(our_mac[i] >> 4) & 0xF];
        out->mac_str[p++] = hx[our_mac[i] & 0xF];
        if (i < 5) out->mac_str[p++] = ':';
    }
    out->mac_str[p] = '\0';
}

int plat_net_ping(const char *host_ip, uint32_t *rtt_ms) {
    uint32_t ip;
    if (net_parse_ip(host_ip, &ip) == 0 && net_icmp_ping(ip, rtt_ms) == 0)
        return 0;
    return net_ping(host_ip, rtt_ms);
}
