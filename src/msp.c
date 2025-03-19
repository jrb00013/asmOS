#include "include/msp.h"

// Disable interrupts (example using inline assembly)
void disable_interrupts(void) {
    __asm__("cli");  // x86 CLI (Clear Interrupt Flag)
}

// Enable interrupts
void enable_interrupts(void) {
    __asm__("sti");  // x86 STI (Set Interrupt Flag)
}

// Halt the system (used for error states or shutdown)
void halt_system(void) {
    while (1) {
        __asm__("hlt");  // x86 HLT (Halt CPU until next interrupt)
    }
}
