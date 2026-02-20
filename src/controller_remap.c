#include "controller_remap.h"
#include "kernel.h"
#include <stddef.h>

static controller_profile_t profiles[CTRL_MAX_PROFILES];
static int profile_count;
static controller_profile_t current;

void controller_remap_init(void) {
    profile_count = 0;
    for (int i = 0; i < CTRL_MAX_PROFILES; i++) {
        profiles[i].name[0] = '\0';
        profiles[i].sensitivity = 128;
        profiles[i].deadzone = 32;
        profiles[i].turbo_mask = 0;
        profiles[i].macro_len = 0;
        for (int j = 0; j < 16; j++) profiles[i].button_remap[j] = j;
    }
    current.sensitivity = 128;
    current.deadzone = 32;
    current.turbo_mask = 0;
    current.macro_len = 0;
    current.name[0] = '\0';
    for (int j = 0; j < 16; j++) current.button_remap[j] = j;
}

static void copy_name(char *dst, const char *src) {
    int i = 0;
    while (i < CTRL_PROFILE_NAME_LEN - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

int controller_profile_save(const char *name) {
    if (!name || !name[0]) return -1;
    if (profile_count >= CTRL_MAX_PROFILES) return -2;
    copy_name(profiles[profile_count].name, name);
    profiles[profile_count].sensitivity = current.sensitivity;
    profiles[profile_count].deadzone = current.deadzone;
    profiles[profile_count].turbo_mask = current.turbo_mask;
    profiles[profile_count].macro_len = current.macro_len;
    for (int i = 0; i < CTRL_MACRO_LEN; i++)
        profiles[profile_count].macro_keys[i] = current.macro_keys[i];
    for (int i = 0; i < 16; i++)
        profiles[profile_count].button_remap[i] = current.button_remap[i];
    profile_count++;
    return 0;
}

int controller_profile_load(const char *name) {
    if (!name || !name[0]) return -1;
    for (int i = 0; i < profile_count; i++) {
        int j = 0;
        while (profiles[i].name[j] == name[j] && name[j]) j++;
        if (!name[j] && !profiles[i].name[j]) {
            current.sensitivity = profiles[i].sensitivity;
            current.deadzone = profiles[i].deadzone;
            current.turbo_mask = profiles[i].turbo_mask;
            current.macro_len = profiles[i].macro_len;
            copy_name(current.name, profiles[i].name);
            for (int k = 0; k < CTRL_MACRO_LEN; k++)
                current.macro_keys[k] = profiles[i].macro_keys[k];
            for (int k = 0; k < 16; k++)
                current.button_remap[k] = profiles[i].button_remap[k];
            return 0;
        }
    }
    return -2;
}

void controller_profile_list(void) {
    kprint("\n  ");
    kprint_color(" controller profiles ", 0x2A);
    kprint_color(" --------------------\n", 0x08);
    if (profile_count == 0) {
        kprint("  (none) Save with: controller profile save <name>\n");
        kprint("  ----------------------------------------\n\n");
        return;
    }
    for (int i = 0; i < profile_count; i++)
        kprintf("  %d. %s (sens=%d dead=%d turbo=0x%X)\n",
                i + 1, profiles[i].name, profiles[i].sensitivity,
                profiles[i].deadzone, (unsigned)profiles[i].turbo_mask);
    kprint("  ----------------------------------------\n\n");
}

int controller_profile_apply(const char *name) {
    int r = controller_profile_load(name);
    if (r == 0)
        kprintf("  Profile '%s' applied.\n", name);
    return r;
}

void controller_set_sensitivity(int value) {
    if (value < 0) value = 0;
    if (value > 255) value = 255;
    current.sensitivity = value;
}

void controller_set_deadzone(int value) {
    if (value < 0) value = 0;
    if (value > 255) value = 255;
    current.deadzone = value;
}

void controller_set_turbo(int mask) {
    current.turbo_mask = mask & 0xFFFF;
}

void controller_remap_print_current(void) {
    kprint("\n  ");
    kprint_color(" controller ", 0x2A);
    kprint_color(" --------------------------------\n", 0x08);
    kprintf("  Profile:   %s\n", current.name[0] ? current.name : "(default)");
    kprintf("  Sensitivity: %d  Deadzone: %d  Turbo: 0x%X\n",
            current.sensitivity, current.deadzone, (unsigned)current.turbo_mask);
    kprint("  ----------------------------------------\n\n");
}
