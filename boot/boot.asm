; Enhanced PS2-compatible x86 bootloader
[bits 16]
[org 0x7C00]

extern disk_error_msg

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
    je load_error

    ; Jump to kernel
    mov si, kernel_loaded_msg
    call print_string
    jmp 0x1000:0000         ; Kernel entry point

load_kernel:
    mov ah, 0x02            ; BIOS read sectors function
    mov al, 15              ; Number of sectors to read (adjust as needed)
    mov ch, 0x00            ; Cylinder 0
    mov dh, 0x00            ; Head 0
    mov cl, 0x02            ; Sector 2 (starts at 1)
    mov bx, 0x0000          ; Offset within segment to store data (start at 0)
    mov es, 0x1000          ; Segment to store data (0x1000 => physical 0x10000)
    mov si, boot_drive
    mov dl, [si]  ; Disk number passed from BIOS (usually 0x00 or 0x80)

read_loop:
    int 0x13                ; BIOS disk read
    jc disk_error           ; Jump if carry set (error)

    ret                     ; Return after successful read

disk_error:
    ; Handle disk read error (could retry or hang)
    mov si, disk_error_msg
    call print_string       ; Assume you have print_string routine
    jmp $


load_error:
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
boot_drive db 0 



; Boot signature
times 510 - ($ - $$) db 0
dw 0xAA55
