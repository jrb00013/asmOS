#include "platform.h"
#include "net.h"
#include <string.h>
#include <kernel.h>
#include <ps2ip.h>
#include <netman.h>
#include <loadfile.h>
#include <stdint.h>

static int net_ready;
static uint32_t our_ip = (10u << 24) | (0u << 16) | (0u << 8) | 2u;

static int load_network_modules(void) {
    int ret;
    ret = SifLoadModule("host:netman.irx", 0, NULL);
    if (ret < 0) return -1;
    ret = SifLoadModule("host:smap.irx", 0, NULL);
    if (ret < 0) return -1;
    return 0;
}

int plat_net_init(void) {
    if (net_ready) return 0;
    if (load_network_modules() != 0) {
        net_ready = 0;
        return net_init();
    }
    NetManInit();
    ps2ipInit();
    ps2ipConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    IP4_ADDR(&cfg.ipaddr, 10, 0, 0, 2);
    IP4_ADDR(&cfg.netmask, 255, 255, 255, 0);
    IP4_ADDR(&cfg.gw, 10, 0, 0, 1);
    ps2ipSetConfig(&cfg);
    net_ready = 1;
    return net_init();
}

void plat_net_shutdown(void) {
    if (net_ready) ps2ipDeinit();
    net_shutdown();
    net_ready = 0;
}

int plat_net_send(const void *data, size_t len) {
    if (!data || len == 0) return -1;
    return (int)len;
}

int plat_net_recv(void *buf, size_t max_len) {
    (void)buf;
    (void)max_len;
    return 0;
}

void plat_net_get_info(plat_net_info_t *out) {
    if (!out) return;
    out->linked = net_ready;
    out->ip = our_ip;
    out->netmask = 0xFFFFFF00u;
    net_ip_to_str(our_ip, out->ip_str, PLAT_IP_STR_MAX);
    strcpy(out->mac_str, "00:00:00:00:00:00");
}

int plat_net_ping(const char *host_ip, uint32_t *rtt_ms) {
    return net_ping(host_ip, rtt_ms);
}
