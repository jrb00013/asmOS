#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <stddef.h>

#define NET_PORT_PING       0
#define NET_PORT_UDP_BASE   9000
#define NET_PORT_NETCTRL    9001
#define NET_PORT_STREAM     9002
#define NET_PORT_PARTY      9003

#define NET_MAX_PAYLOAD     1400
#define NET_HOST_MAX        64

int net_init(void);
void net_shutdown(void);
int net_udp_send(uint32_t dst_ip, uint16_t dst_port, const void *data, uint16_t len);
int net_udp_recv(uint32_t *src_ip, uint16_t *src_port, void *buf, uint16_t max_len);
int net_ping(const char *host, uint32_t *rtt_ms);
int net_icmp_ping(uint32_t dst_ip, uint32_t *rtt_ms);
int net_parse_ip(const char *str, uint32_t *out);
void net_ip_to_str(uint32_t ip, char *buf, int max);

/* Minimal TCP-like stream (single connection, blocking) */
typedef struct net_stream net_stream_t;
net_stream_t *net_connect(const char *host, uint16_t port);
int net_stream_send(net_stream_t *s, const void *data, size_t len);
int net_stream_recv(net_stream_t *s, void *buf, size_t max_len);
void net_stream_close(net_stream_t *s);

#endif /* NET_H */
