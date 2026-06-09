/*
 * PC keyboard driver: poll port 0x60 (data) and 0x64 (status).
 * Used when not building for PS2 (no PS2_HARDWARE).
 */
#include "keyboard.h"
#include "arch_x86.h"

#ifndef PS2_HARDWARE

/* Bit 0 of status = output buffer full (data available at 0x60) */
int keyboard_has_key(void) {
    return (inb(0x64) & 1) != 0;
}

uint8_t keyboard_get_scancode(void) {
    if (!keyboard_has_key())
        return 0;
    return inb(0x60);
}

#else

int keyboard_has_key(void) { (void)0; return 0; }
uint8_t keyboard_get_scancode(void) { (void)0; return 0; }

#endif
