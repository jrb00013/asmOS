#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/* PC keyboard via ports 0x60/0x64. No-op when PS2_HARDWARE (use controller). */

/* Returns 1 if a key event is available in the controller buffer. */
int keyboard_has_key(void);

/* Returns next scancode (make or break). 0 if no data. Break = 0x80 | make. */
uint8_t keyboard_get_scancode(void);

/* Scancode set 1 (XT): arrow keys and common keys */
#define SCAN_UP    0x48
#define SCAN_DOWN  0x50
#define SCAN_LEFT  0x4B
#define SCAN_RIGHT 0x4D
#define SCAN_ENTER 0x1C
#define SCAN_SPACE 0x39
#define SCAN_ESC   0x01

#endif
