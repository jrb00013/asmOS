#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stdint.h>
#include <stddef.h>

/*
 * Lightweight network transmission layer for PS2 Command Core.
 * Used by party, streaming, and other network features.
 * Constraints: 32MB RAM, low latency, packet batching, error correction.
 */

#define TRANSPORT_HEADER_SIZE    12
#define TRANSPORT_MAX_PAYLOAD    1400
#define TRANSPORT_PACKET_MAX     (TRANSPORT_HEADER_SIZE + TRANSPORT_MAX_PAYLOAD)
#define TRANSPORT_BATCH_MAX      4
#define TRANSPORT_HEARTBEAT_MS   2000
#define TRANSPORT_RECONNECT_MS   5000

/* Packet types */
#define TRANSPORT_TYPE_DATA      0x01
#define TRANSPORT_TYPE_ACK       0x02
#define TRANSPORT_TYPE_HEARTBEAT 0x03
#define TRANSPORT_TYPE_FIN       0x04

typedef struct {
    uint8_t  type;
    uint8_t  flags;
    uint16_t seq;
    uint32_t len;
    uint16_t crc;
    uint8_t  reserved[2];
} transport_header_t;

typedef struct {
    transport_header_t hdr;
    uint8_t payload[TRANSPORT_MAX_PAYLOAD];
} transport_packet_t;

/* Session state for failover reconnection */
typedef struct {
    uint16_t local_seq;
    uint16_t remote_seq;
    uint8_t  connected;
    uint8_t  heartbeat_missed;
    uint32_t last_heartbeat_ticks;
} transport_session_t;

/* Initialize transport layer; call after network init. */
int transport_init(void);

/* Shutdown and free resources. */
void transport_shutdown(void);

/* Send one packet (header + payload). Payload length 0..TRANSPORT_MAX_PAYLOAD. */
int transport_send(uint8_t type, const void *payload, uint16_t len);

/* Send multiple payloads in one batch (reduces overhead). */
int transport_send_batch(const void *payloads[], const uint16_t lens[], int count);

/* Receive into buffer; returns payload length or -1 on error. */
int transport_receive(void *buffer, size_t max_len, uint8_t *out_type);

/* Run heartbeat and failover; call periodically from main loop or timer. */
void transport_tick(void);

/* Request reconnection (e.g. after failure). */
void transport_request_reconnect(void);

/* Get current session state for CLI/debug. */
void transport_get_session(transport_session_t *out);

#endif /* TRANSPORT_H */
