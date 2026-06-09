; x86 port I/O primitives shared by kernel C code.
[BITS 32]
section .note.GNU-stack noalloc noexec nowrite progbits
section .text

global inb
global outb
global inw
global outw

; uint8_t inb(uint16_t port)
inb:
    mov edx, [esp + 4]
    in al, dx
    ret

; void outb(uint16_t port, uint8_t value)
outb:
    mov edx, [esp + 4]
    mov eax, [esp + 8]
    out dx, al
    ret

; uint16_t inw(uint16_t port)
inw:
    mov edx, [esp + 4]
    in ax, dx
    ret

; void outw(uint16_t port, uint16_t value)
outw:
    mov edx, [esp + 4]
    mov eax, [esp + 8]
    out dx, ax
    ret
