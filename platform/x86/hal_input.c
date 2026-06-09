/* x86 platform HAL — keyboard input (no fake controller stubs). */

#include "platform.h"
#include "keyboard.h"
#include <stdint.h>

extern uint8_t scancode_to_ascii(uint8_t sc, uint32_t shift);

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
    if (!keyboard_has_key()) return 0;
    uint8_t sc = keyboard_get_scancode();
    static int shift;
    if (sc == 0x2A || sc == 0x36) { shift = 1; return 0; }
    if (sc == 0xAA || sc == 0xB6) { shift = 0; return 0; }
    char c = (char)scancode_to_ascii(sc, (uint32_t)shift);
    if (!c) return 0;
    out->present = 1;
    out->buttons = (uint16_t)c;
    return 0;
}
