#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <stdint.h>

/*
 * Bluetooth stack (optional): external USB BT adapter.
 * HID translation, controller virtualization, latency compensation.
 * Supports: PS3/PS4/PS5, Xbox 360/One, generic PC controllers.
 */

#define BT_ADDR_STR_MAX   18
#define BT_DEVICE_MAX     6
#define BT_PROFILE_NAME_MAX 24
#define BT_LOGICAL_BUTTONS 16
#define BT_PROFILE_SLOTS   4

typedef struct {
    char addr[BT_ADDR_STR_MAX];
    uint8_t paired;
    uint8_t connected;
} bt_device_t;

/* Initialize Bluetooth stack (no-op if no USB BT adapter). */
int bt_init(void);

/* Scan for devices; results reported via callback or CLI. */
int bt_scan(void);

/* Pair with device by address (e.g. "00:11:22:33:44:55"). */
int bt_pair(const char *addr);

/* Unpair device. */
int bt_unpair(const char *addr);

/* List paired/connected devices. */
int bt_list(bt_device_t *out, int max_count);

/* Controller mapping: show or set virtual pad mapping. */
int controller_map_show(void);
int controller_map_set(int logical_button, int physical_source);

/* Controller profile: create/save and load. */
int controller_profile_create(const char *name);
int controller_profile_load(const char *name);

#endif /* BLUETOOTH_H */
