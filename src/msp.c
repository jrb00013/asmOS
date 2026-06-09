#include "msp.h"

extern void disable_interrupts_asm(void);
extern void enable_interrupts_asm(void);

void disable_interrupts(void) {
#ifndef PLATFORM_PS2
    disable_interrupts_asm();
#endif
}

void enable_interrupts(void) {
#ifndef PLATFORM_PS2
    enable_interrupts_asm();
#endif
}
