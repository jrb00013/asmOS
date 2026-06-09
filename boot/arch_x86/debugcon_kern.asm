; QEMU isa-debugcon (port 0xE9) — 32-bit kernel.
[BITS 32]
section .text

global debugcon_putc

; void debugcon_putc(char c)
debugcon_putc:
    mov eax, [esp + 4]
    mov dx, 0xE9
    out dx, al
    ret
