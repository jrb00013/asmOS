/* No built-in Bluetooth workaround: USB BT dongle stub. */

#include "bt_dongle.h"
#include "kernel.h"
#include <stddef.h>

static int probed = 0;
static int inited = 0;
static uint16_t last_buttons = 0;
static int8_t last_lx = 0, last_ly = 0, last_rx = 0, last_ry = 0;

bt_dongle_result_t bt_dongle_probe(void) {
    /* Real: scan USB for device with Bluetooth class. */
    probed = 1;
    return BT_DONGLE_NO_DEVICE;  /* stub: no dongle */
}

bt_dongle_result_t bt_dongle_init(void) {
    if (!probed && bt_dongle_probe() != BT_DONGLE_OK) return BT_DONGLE_NO_DEVICE;
    inited = 1;
    return BT_DONGLE_OK;
}

bt_dongle_result_t bt_dongle_send_cmd(const uint8_t *cmd, uint32_t len) {
    (void)cmd;
    (void)len;
    if (!inited) return BT_DONGLE_NO_DEVICE;
    return BT_DONGLE_OK;
}

int bt_dongle_poll_hid(uint8_t *report, int max_len) {
    (void)report;
    (void)max_len;
    return 0;  /* no data */
}

void bt_dongle_get_controller_state(uint16_t *buttons, int8_t *lx, int8_t *ly, int8_t *rx, int8_t *ry) {
    if (buttons) *buttons = last_buttons;
    if (lx) *lx = last_lx;
    if (ly) *ly = last_ly;
    if (rx) *rx = last_rx;
    if (ry) *ry = last_ry;
}
