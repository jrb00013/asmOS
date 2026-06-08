#include "bt_dongle.h"
#include "kernel.h"

static int dongle_present = 1;

bt_dongle_result_t bt_dongle_probe(void) {
    dongle_present = 1;
    return BT_DONGLE_OK;
}

bt_dongle_result_t bt_dongle_init(void) {
    return bt_dongle_probe();
}

bt_dongle_result_t bt_dongle_send_cmd(const uint8_t *cmd, uint32_t len) {
    (void)cmd;
    (void)len;
    return dongle_present ? BT_DONGLE_OK : BT_DONGLE_NO_DEVICE;
}

int bt_dongle_poll_hid(uint8_t *report, int max_len) {
    if (!dongle_present || !report || max_len < 6) return 0;
    report[0] = 0;
    report[1] = 0;
    report[2] = 0;
    report[3] = 0;
    report[4] = 0;
    report[5] = 0;
    return 6;
}

void bt_dongle_get_controller_state(uint16_t *buttons, int8_t *lx, int8_t *ly, int8_t *rx, int8_t *ry) {
    if (buttons) *buttons = 0;
    if (lx) *lx = 0;
    if (ly) *ly = 0;
    if (rx) *rx = 0;
    if (ry) *ry = 0;
}
