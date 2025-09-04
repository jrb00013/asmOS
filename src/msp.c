#include "msp.h"

// Disable interrupts (example using inline assembly)
void disable_interrupts(void) {
    __asm__("cli");  // x86 CLI (Clear Interrupt Flag)
}

// Enable interrupts
void enable_interrupts(void) {
    __asm__("sti");  // x86 STI (Set Interrupt Flag)
}
