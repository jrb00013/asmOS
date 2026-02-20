/* Virtual controller over network — cross-controller without BT. */

#include "network_controller.h"
#include "kernel.h"
#include <stddef.h>

static int running = 0;
static uint16_t last_buttons = 0;
static int8_t last_lx = 0, last_ly = 0, last_rx = 0, last_ry = 0;

int network_controller_start(void) {
    /* Real: bind UDP socket to NETCTRL_PORT, spawn task or poll in loop. */
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
    /* Real: recvfrom(); parse NETCTRL_MAGIC packet; update last_*. */
    return 0;
}
