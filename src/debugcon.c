/* QEMU isa-debugcon (port 0xE9) — x86 boot verification only. */
#ifndef PLATFORM_PS2

#include <stdint.h>

void debugcon_putc(char c) {
    __asm__ volatile ("outb %0, $0xE9" : : "a"(c));
}

void debugcon_puts(const char *s) {
    if (!s) return;
    while (*s) debugcon_putc(*s++);
}

#endif
