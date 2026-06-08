/* x86 platform HAL — keyboard and controller input. */

#include "platform.h"
#include "keyboard.h"
#include <stdint.h>

extern void init_ps2_controllers(void);

int plat_keyboard_scancode(void) {
    return (int)keyboard_get_scancode();
}

int plat_keyboard_has_key(void) {
    return keyboard_has_key();
}

int plat_controller_read(int port, plat_controller_state_t *out) {
    if (!out) return -1;
    out->present = 0;
    out->buttons = 0;
    out->lx = out->ly = out->rx = out->ry = 0;
    if (port != 0) return -1;
    uint8_t data[6];
    asm volatile("call sys_ps2_controller_read" : : "a"(data) : "memory");
    out->present = 1;
    out->buttons = (uint16_t)(data[0] | (data[1] << 8));
    out->lx = (int8_t)data[2];
    out->ly = (int8_t)data[3];
    out->rx = (int8_t)data[4];
    out->ry = (int8_t)data[5];
    return 0;
}
