; print.asm

[bits 16]

print_string:
    pusha
    mov ah, 0x0E  ; Teletype mode

.print_char:
    lodsb         ; Load next char from [SI] into AL
    cmp al, 0
    je .done
    int 0x10      ; BIOS print char
    jmp .print_char

.done:
    popa
    ret
