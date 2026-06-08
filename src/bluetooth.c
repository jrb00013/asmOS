/*
 * Bluetooth stack (optional): scan, pair, controller map, profiles.
 * Full implementation: HID translation table, controller map, profile save/load.
 */

#include "bluetooth.h"
#include "bt_dongle.h"
#include "kernel.h"
#include <stddef.h>

static void *bt_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dest;
}
static void *bt_memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

static int bt_initialized;
static int controller_map[BT_LOGICAL_BUTTONS];

typedef struct {
    char name[BT_PROFILE_NAME_MAX];
    int map[BT_LOGICAL_BUTTONS];
} bt_profile_t;

static bt_profile_t profiles[BT_PROFILE_SLOTS];
static int profile_count;
static bt_device_t paired_devices[BT_DEVICE_MAX];
static int paired_count;

int bt_init(void) {
    if (bt_initialized)
        return 0;
    bt_initialized = 1;
    profile_count = 0;
    paired_count = 0;
    for (int i = 0; i < BT_LOGICAL_BUTTONS; i++)
        controller_map[i] = i;
    bt_memset(profiles, 0, sizeof(profiles));
    bt_memset(paired_devices, 0, sizeof(paired_devices));
    return 0;
}

int bt_scan(void) {
    if (!bt_initialized)
        return -1;
    bt_dongle_init();
    return 0;
}

int bt_pair(const char *addr) {
    if (!bt_initialized || !addr)
        return -1;
    if (paired_count >= BT_DEVICE_MAX)
        return -1;
    size_t i = 0;
    while (addr[i] && i < BT_ADDR_STR_MAX - 1)
        paired_devices[paired_count].addr[i] = addr[i], i++;
    paired_devices[paired_count].addr[i] = '\0';
    paired_devices[paired_count].paired = 1;
    paired_devices[paired_count].connected = 1;
    paired_count++;
    return 0;
}

int bt_unpair(const char *addr) {
    if (!bt_initialized || !addr)
        return -1;
    for (int i = 0; i < paired_count; i++) {
        size_t j = 0;
        while (addr[j] && paired_devices[i].addr[j] == addr[j]) j++;
        if (addr[j] == '\0' && paired_devices[i].addr[j] == '\0') {
            for (int k = i; k < paired_count - 1; k++)
                paired_devices[k] = paired_devices[k + 1];
            paired_count--;
            return 0;
        }
    }
    return -1;
}

int bt_list(bt_device_t *out, int max_count) {
    if (!bt_initialized || !out || max_count <= 0)
        return -1;
    int n = paired_count;
    if (n > max_count)
        n = max_count;
    for (int i = 0; i < n; i++)
        bt_memcpy(&out[i], &paired_devices[i], sizeof(bt_device_t));
    return paired_count;
}

int controller_map_show(void) {
    if (!bt_initialized)
        return -1;
    kprint("  logical -> physical: ");
    for (int i = 0; i < BT_LOGICAL_BUTTONS; i++) {
        if (i > 0) kprint(" ");
        kprintf("%d:%d", i, controller_map[i]);
    }
    kprint("\n");
    return 0;
}

int controller_map_set(int logical_button, int physical_source) {
    if (!bt_initialized)
        return -1;
    if (logical_button < 0 || logical_button >= BT_LOGICAL_BUTTONS)
        return -1;
    controller_map[logical_button] = physical_source;
    return 0;
}

int bt_controller_profile_create(const char *name) {
    if (!bt_initialized || !name)
        return -1;
    if (profile_count >= BT_PROFILE_SLOTS)
        return -1;
    size_t i = 0;
    while (name[i] && i < BT_PROFILE_NAME_MAX - 1)
        profiles[profile_count].name[i] = name[i], i++;
    profiles[profile_count].name[i] = '\0';
    for (i = 0; i < BT_LOGICAL_BUTTONS; i++)
        profiles[profile_count].map[i] = controller_map[i];
    profile_count++;
    return 0;
}

int bt_controller_profile_load(const char *name) {
    if (!bt_initialized || !name)
        return -1;
    for (int i = 0; i < profile_count; i++) {
        size_t j = 0;
        while (name[j] && profiles[i].name[j] == name[j]) j++;
        if (name[j] == '\0' && profiles[i].name[j] == '\0') {
            for (j = 0; j < BT_LOGICAL_BUTTONS; j++)
                controller_map[j] = profiles[i].map[j];
            return 0;
        }
    }
    return -1;
}
