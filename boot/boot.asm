[bits 16]
[org 0x7C00]

jmp start
nop

; === BIOS Parameter Block (BPB) for PS2 ===
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

; === Bootloader starts here ===
start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [boot_drive], dl

    ; Clear screen
    mov ax, 0x0003
    int 0x10

    ; Display boot message
    mov si, boot_msg
    call print_string

    ; Load kernel
    call load_kernel

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

; === Kernel Loading ===
load_kernel:
    pusha
    
    ; Reset disk system
    mov ah, 0x00
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    ; Load kernel sectors
    mov ah, 0x02
    mov al, 32              ; Load 32 sectors (16KB)
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

; === String Printing ===
print_string:
    lodsb
    or al, al
    jz done
    mov ah, 0x0E
    int 0x10
    jmp print_string

done:
    ret

; === Data Section ===
boot_msg db "PS2 x86 Bootloader v3.0", 0x0D, 0x0A, 0
kernel_loaded_msg db "Kernel loaded! Transferring control...", 0x0D, 0x0A, 0
load_error_msg db "Error loading kernel!", 0x0D, 0x0A, 0
disk_error_msg db "Disk read error!", 0x0D, 0x0A, 0
kernel_corrupt_msg db "Kernel corrupted!", 0x0D, 0x0A, 0

boot_drive db 0
kernel_size dw 0

; === Boot sector padding & signature ===
times 510 - ($ - $$) db 0
dw 0xAA55
