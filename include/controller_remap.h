#ifndef CONTROLLER_REMAP_H
#define CONTROLLER_REMAP_H

#include <stdint.h>

#define CTRL_PROFILE_NAME_LEN  24
#define CTRL_MAX_PROFILES      8
#define CTRL_MACRO_LEN         32

typedef struct {
    char name[CTRL_PROFILE_NAME_LEN];
    int button_remap[16];
    int sensitivity;
    int deadzone;
    int turbo_mask;
    uint8_t macro_len;
    uint8_t macro_keys[CTRL_MACRO_LEN];
} controller_profile_t;

void controller_remap_init(void);
int controller_profile_save(const char *name);
int controller_profile_load(const char *name);
void controller_profile_list(void);
int controller_profile_apply(const char *name);
void controller_remap_print_current(void);
void controller_set_sensitivity(int value);
void controller_set_deadzone(int value);
void controller_set_turbo(int mask);

#endif
