; Enhanced PS2-compatible x86 bootloader
[bits 16]
[org 0x7C00]

; Bootloader entry point
start:
    cli                     ; Disable interrupts during setup
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00          ; Stack grows down from bootloader
    sti                     ; Re-enable interrupts

    ; Clear screen
    mov ax, 0x0003          ; 80x25 text mode
    int 0x10

    ; Print boot message
    mov si, boot_msg
    call print_string

    ; Load kernel from disk (FAT12)
    call load_kernel

    ; Check if kernel loaded successfully
    cmp word [kernel_size], 0
    je .load_error

    ; Jump to kernel
    mov si, kernel_loaded_msg
    call print_string
    jmp 0x1000:0000         ; Kernel entry point

.load_error:
    mov si, load_error_msg
    call print_string
    jmp $

; Includes
%include "fat12.asm"
%include "disk.asm"
%include "print.asm"

; Data
boot_msg db "PS2 x86 Bootloader v1.0", 0x0D, 0x0A, 0
kernel_loaded_msg db "Kernel loaded, transferring control...", 0x0D, 0x0A, 0
load_error_msg db "Error loading kernel!", 0x0D, 0x0A, 0
kernel_size dw 0

; Boot signature
times 510 - ($ - $$) db 0
dw 0xAA55
