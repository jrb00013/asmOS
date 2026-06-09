#include "msp.h"

void disable_interrupts(void) {
#ifndef PLATFORM_PS2
    __asm__("cli");
#endif
}

void enable_interrupts(void) {
#ifndef PLATFORM_PS2
    __asm__("sti");
#endif
}
