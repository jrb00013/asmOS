#include "network_controller.h"
#include "net.h"
#include "kernel.h"
#include <stddef.h>

static int running = 0;
static uint16_t last_buttons = 0;
static int8_t last_lx = 0, last_ly = 0, last_rx = 0, last_ry = 0;

int network_controller_start(void) {
    running = 1;
    return 0;
}

void network_controller_stop(void) {
    running = 0;
}

void network_controller_get_state(uint16_t *buttons, int8_t *lx, int8_t *ly, int8_t *rx, int8_t *ry) {
    if (buttons) *buttons = last_buttons;
    if (lx) *lx = last_lx;
    if (ly) *ly = last_ly;
    if (rx) *rx = last_rx;
    if (ry) *ry = last_ry;
}

int network_controller_poll(void) {
    if (!running) return 0;
    uint8_t buf[64];
    uint32_t sip;
    uint16_t sport;
    int n = net_udp_recv(&sip, &sport, buf, sizeof(buf));
    if (n < (int)sizeof(network_controller_packet_t)) return 0;
    network_controller_packet_t *p = (network_controller_packet_t *)buf;
    if (p->magic != NETCTRL_MAGIC) return 0;
    last_buttons = p->buttons;
    last_lx = p->lx;
    last_ly = p->ly;
    last_rx = p->rx;
    last_ry = p->ry;
    return 1;
}
