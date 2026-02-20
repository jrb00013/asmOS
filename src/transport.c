/*
 * Lightweight transport layer: batching, heartbeat, failover, ACK.
 * Uses sys_network_send / sys_network_receive via asm (no C impl in tree).
 */

#include "transport.h"
#include "ps2_hardware.h"
#include <stddef.h>

static void *transport_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dest;
}
static void *transport_memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}
#define memcpy transport_memcpy
#define memset transport_memset

/* Call asm syscalls: eax = ptr, ebx = length (see syscalls.asm). */
static int do_network_send(const void *data, size_t len) {
    asm volatile("call sys_network_send" : : "a"(data), "b"(len) : "memory");
    return 0;
}
static int do_network_receive(void *buf, size_t max_len) {
    (void)buf;
    (void)max_len;
    asm volatile("call sys_network_receive" : : "a"(buf), "b"(max_len) : "memory");
    return 0;
}

static transport_session_t session;
static uint8_t tx_buf[TRANSPORT_PACKET_MAX];
static uint8_t rx_buf[TRANSPORT_PACKET_MAX];
static int initialized;
static uint32_t transport_ticks;
#define TRANSPORT_HEARTBEAT_TICKS   (TRANSPORT_HEARTBEAT_MS / 10)
#define TRANSPORT_RECONNECT_TICKS   (TRANSPORT_RECONNECT_MS / 10)
#define TRANSPORT_MAX_MISSED_HEARTBEAT 3
#define TRANSPORT_MAX_RECONNECT_ATTEMPTS 10

static uint16_t crc16_simple(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc ^= (uint16_t)*data++;
        for (int i = 0; i < 8; i++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001U : (crc >> 1);
    }
    return crc;
}

int transport_init(void) {
    if (initialized)
        return 0;
    memset(&session, 0, sizeof(session));
    transport_ticks = 0;
    initialized = 1;
    session.connected = 1;
    return 0;
}

void transport_shutdown(void) {
    initialized = 0;
    session.connected = 0;
}

int transport_send(uint8_t type, const void *payload, uint16_t len) {
    if (!initialized || len > TRANSPORT_MAX_PAYLOAD)
        return -1;

    transport_header_t *h = (transport_header_t *)tx_buf;
    h->type   = type;
    h->flags  = 0;
    h->seq    = session.local_seq++;
    h->len    = len;
    h->crc    = 0;
    h->reserved[0] = h->reserved[1] = 0;

    if (payload && len)
        memcpy(tx_buf + TRANSPORT_HEADER_SIZE, payload, len);
    h->crc = crc16_simple(tx_buf + TRANSPORT_HEADER_SIZE, len);

    size_t total = TRANSPORT_HEADER_SIZE + len;
    if (do_network_send(tx_buf, total) != 0)
        return -1;
    return (int)len;
}

/* Batch: encode count + lengths + payloads into one packet when possible. */
int transport_send_batch(const void *payloads[], const uint16_t lens[], int count) {
    if (!initialized || count <= 0 || count > TRANSPORT_BATCH_MAX)
        return -1;
    uint16_t total_len = 0;
    for (int i = 0; i < count; i++)
        total_len += 2 + lens[i];
    if (total_len > TRANSPORT_MAX_PAYLOAD - 2)
        return -1;
    uint8_t batch_buf[TRANSPORT_MAX_PAYLOAD];
    uint8_t *p = batch_buf;
    *p++ = (uint8_t)count;
    *p++ = 0;
    for (int i = 0; i < count; i++) {
        *p++ = (uint8_t)(lens[i] & 0xFF);
        *p++ = (uint8_t)(lens[i] >> 8);
        if (lens[i] && payloads[i])
            memcpy(p, payloads[i], lens[i]);
        p += lens[i];
    }
    return transport_send(TRANSPORT_TYPE_DATA, batch_buf, (uint16_t)(p - batch_buf));
}

int transport_receive(void *buffer, size_t max_len, uint8_t *out_type) {
    if (!initialized || !buffer || !out_type)
        return -1;

    int n = do_network_receive(rx_buf, sizeof(rx_buf));
    if (n < (int)TRANSPORT_HEADER_SIZE)
        return -1;

    transport_header_t *h = (transport_header_t *)rx_buf;
    uint16_t len = h->len;
    if (len > TRANSPORT_MAX_PAYLOAD || (size_t)len > max_len)
        return -1;

    uint16_t crc = crc16_simple(rx_buf + TRANSPORT_HEADER_SIZE, len);
    if (crc != h->crc)
        return -1;

    *out_type = h->type;
    session.remote_seq = h->seq;
    session.last_heartbeat_ticks = transport_ticks;
    session.heartbeat_missed = 0;
    if (len)
        memcpy(buffer, rx_buf + TRANSPORT_HEADER_SIZE, len);

    if (h->type == TRANSPORT_TYPE_DATA) {
        transport_header_t *ack_h = (transport_header_t *)tx_buf;
        ack_h->type = TRANSPORT_TYPE_ACK;
        ack_h->flags = 0;
        ack_h->seq = h->seq;
        ack_h->len = 0;
        ack_h->crc = 0;
        ack_h->reserved[0] = ack_h->reserved[1] = 0;
        do_network_send(tx_buf, TRANSPORT_HEADER_SIZE);
    }
    return (int)len;
}

void transport_tick(void) {
    if (!initialized)
        return;
    transport_ticks++;

    if (session.connected) {
        if (transport_ticks - session.last_heartbeat_ticks >= TRANSPORT_HEARTBEAT_TICKS) {
            session.heartbeat_missed++;
            if (session.heartbeat_missed >= TRANSPORT_MAX_MISSED_HEARTBEAT)
                session.connected = 0;
            else
                transport_send(TRANSPORT_TYPE_HEARTBEAT, 0, 0);
        }
        return;
    }

    static uint32_t next_reconnect_tick;
    static int reconnect_attempts;
    if (reconnect_attempts == 0)
        next_reconnect_tick = transport_ticks + TRANSPORT_RECONNECT_TICKS;
    if (transport_ticks >= next_reconnect_tick && reconnect_attempts < TRANSPORT_MAX_RECONNECT_ATTEMPTS) {
        reconnect_attempts++;
        session.connected = 1;
        session.heartbeat_missed = 0;
        session.last_heartbeat_ticks = transport_ticks;
        next_reconnect_tick = transport_ticks + TRANSPORT_RECONNECT_TICKS * (reconnect_attempts > 4 ? 2 : 1);
    }
}

void transport_request_reconnect(void) {
    session.connected = 0;
    session.heartbeat_missed = TRANSPORT_MAX_MISSED_HEARTBEAT;
}

void transport_get_session(transport_session_t *out) {
    if (out)
        memcpy(out, &session, sizeof(transport_session_t));
}
