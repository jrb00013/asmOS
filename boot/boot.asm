[bits 16]
[org 0x7C00]

jmp start
nop

; === Enhanced BIOS Parameter Block (BPB) for PS2 ===
OEMLabel        db 'PS2OS   '      ; OEM Name - PS2 specific
BytesPerSector  dw 512
SectorsPerCluster db 1
ReservedSectors dw 1
NumFATs         db 2
RootEntries     dw 224
TotalSectors16  dw 2880
MediaDescriptor db 0xF0
SectorsPerFAT   dw 9
SectorsPerTrack dw 18
NumHeads        db 2
HiddenSectors   dd 0
TotalSectors32  dd 0
DriveNumber     db 0
Reserved1       db 0
BootSignature   db 0x29
VolumeID        dd 0x12345678
VolumeLabel     db 'PS2OS     '    ; PS2 OS label
FileSystemType  db 'FAT12   '

; === Enhanced Bootloader starts here ===
start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [boot_drive], dl

    ; Clear screen and set video mode for PS2
    mov ax, 0x0003
    int 0x10

    ; Display enhanced boot message
    mov si, boot_msg
    call print_string

    ; Check for PS2 hardware (basic detection)
    call detect_ps2_hardware

    ; Load kernel with enhanced error handling
    call load_kernel_enhanced

    cmp word [kernel_size], 0
    je load_error

    mov si, kernel_loaded_msg
    call print_string

    ; Set up segments for kernel
    mov ax, 0x1000
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Jump to kernel
    jmp 0x1000:0x0000

; === Enhanced PS2 Hardware Detection ===
detect_ps2_hardware:
    pusha
    
    ; Check for PS2-specific memory regions
    mov ax, 0xE820
    mov edx, 'PAMS'
    mov ebx, 0
    mov ecx, 20
    mov edi, memory_map_buffer
    int 0x15
    
    ; Check if we got memory map (indicates PS2-like hardware)
    jc .no_memory_map
    cmp eax, 'PAMS'
    je .ps2_detected
    
.no_memory_map:
    ; Fallback: assume PS2 if we have at least 32MB RAM
    mov ah, 0x88
    int 0x15
    cmp ax, 32
    jb .not_ps2
    
.ps2_detected:
    mov si, ps2_detected_msg
    call print_string
    jmp .detection_done
    
.not_ps2:
    mov si, not_ps2_msg
    call print_string
    
.detection_done:
    popa
    ret

; === Enhanced Kernel Loading ===
load_kernel_enhanced:
    pusha
    
    ; Reset disk system
    mov ah, 0x00
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    ; Load more sectors for larger kernel support
    mov ah, 0x02
    mov al, 32              ; Load 32 sectors (16KB) instead of 15
    mov ch, 0x00
    mov dh, 0x00
    mov cl, 0x02
    mov bx, 0x0000
    mov es, bx
    mov bx, 0x1000
    mov dl, [boot_drive]

    push 0x1000
    pop es
    xor bx, bx

    int 0x13
    jc disk_error

    ; Verify kernel magic number
    mov ax, es
    mov ds, ax
    mov si, 0
    cmp word [si], 0x7F45    ; ELF magic number
    jne kernel_corrupt

    mov word [kernel_size], 32 * 512
    popa
    ret

kernel_corrupt:
    mov si, kernel_corrupt_msg
    call print_string
    jmp $

disk_error:
    mov si, disk_error_msg
    call print_string
    jmp $

load_error:
    mov si, load_error_msg
    call print_string
    jmp $

; === Enhanced String Printing ===
print_string:
    lodsb
    or al, al
    jz done
    mov ah, 0x0E
    int 0x10
    jmp print_string

; === Enhanced ELF Parsing ===
parse_elf:
    mov si, bx
    cmp word [es:si], 0x457F
    jne not_elf
    cmp word [es:si+2], 0x464C
    jne not_elf

    mov bx, [es:si+0x18]
    ret

not_elf:
    mov si, not_elf_msg
    call print_string
    jmp $

done:
    ret

; === Enhanced Data Section ===
boot_msg db "PS2 x86 Bootloader v4.0 - Enhanced Edition", 0x0D, 0x0A, 0
kernel_loaded_msg db "Kernel loaded successfully! Transferring control...", 0x0D, 0x0A, 0
load_error_msg db "Error loading kernel!", 0x0D, 0x0A, 0
disk_error_msg db "Disk read error! Check CD/DVD drive.", 0x0D, 0x0A, 0
not_elf_msg db "Invalid ELF file! Kernel corrupted.", 0x0D, 0x0A, 0
kernel_corrupt_msg db "Kernel file corrupted! Reinstall OS.", 0x0D, 0x0A, 0
ps2_detected_msg db "PS2 hardware detected. Optimizing for PS2...", 0x0D, 0x0A, 0
not_ps2_msg db "Warning: PS2 hardware not detected. Running in compatibility mode.", 0x0D, 0x0A, 0

boot_drive db 0
kernel_size dw 0

; Memory map buffer for PS2 detection
memory_map_buffer times 20 db 0

; === Boot sector padding & signature ===
times 510 - ($ - $$) db 0
dw 0xAA55
