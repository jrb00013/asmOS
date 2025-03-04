; boot.asm
; This is a basic bootloader that will initialize the system and load the kernel.

[bits 16]              ; 16-bit mode for the bootloader
[org 0x7C00]           ; Start at the bootloader address

start:
    ; Initialize video mode, clear screen
    mov ah, 0x0E
    mov al, '>'          ; Print '>' for prompt
    int 0x10             ; BIOS interrupt for video

    ; Load the kernel (simple example)
    ; In a real case, we would load the kernel from disk.
    mov ax, 0x0200        ; Set up segment for kernel
    mov ds, ax

    ; Jump to kernel entry point (kernel entry would be at address 0x1000)
    jmp 0x1000

times 510 - ($ - $$) db 0  ; Pad to 510 bytes
dw 0xAA55                 ; Boot signature (required by BIOS)
