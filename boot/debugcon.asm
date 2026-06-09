; QEMU isa-debugcon output (I/O port 0xE9) for boot verification
[BITS 16]

%ifndef DEBUGCON_PORT
%define DEBUGCON_PORT 0xE9
%endif

; void debug_putc(char c) — clobber: ax
global debug_putc
debug_putc:
    mov dx, DEBUGCON_PORT
    out dx, al
    ret

; void debug_puts(si=string)
global debug_puts
debug_puts:
    lodsb
    or al, al
    jz .done
    call debug_putc
    jmp debug_puts
.done:
    ret
