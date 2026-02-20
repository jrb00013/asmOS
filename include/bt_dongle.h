#ifndef BT_DONGLE_H
#define BT_DONGLE_H

#include <stdint.h>

/* No built-in Bluetooth workaround: USB BT dongle + minimal stack.
 * We treat the dongle as a USB HID device; controller data comes
 * as HID reports. */

#define BT_DONGLE_CMD_MAX  64
#define BT_HID_REPORT_MAX  32

typedef enum {
    BT_DONGLE_OK = 0,
    BT_DONGLE_NO_DEVICE,
    BT_DONGLE_ERR_IO,
    BT_DONGLE_ERR_TIMEOUT,
} bt_dongle_result_t;

/* Probe for USB Bluetooth dongle. */
bt_dongle_result_t bt_dongle_probe(void);

/* Init HCI (reset, read BD addr). */
bt_dongle_result_t bt_dongle_init(void);

/* Send HCI command (stub: no real USB). */
bt_dongle_result_t bt_dongle_send_cmd(const uint8_t *cmd, uint32_t len);

/* Poll for HID report (controller input). Returns bytes read or 0. */
int bt_dongle_poll_hid(uint8_t *report, int max_len);

/* Get last paired controller state (buttons, axes). For integration with controller layer. */
void bt_dongle_get_controller_state(uint16_t *buttons, int8_t *lx, int8_t *ly, int8_t *rx, int8_t *ry);

#endif /* BT_DONGLE_H */
