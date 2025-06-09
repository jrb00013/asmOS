[bits 16]
[org 0x7C00]

jmp start
nop

; === BIOS Parameter Block (BPB) ===
OEMLabel        db 'MSWIN4.1'      ; OEM Name
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
VolumeLabel     db 'NO NAME    '
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

    mov ax, 0x0003
    int 0x10

    mov si, boot_msg
    call print_string

    call load_kernel

    cmp word [kernel_size], 0
    je load_error

    mov si, kernel_loaded_msg
    call print_string

    mov ax, 0x1000
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    jmp 0x1000:0x0000

load_kernel:
    mov ah, 0x02
    mov al, 15
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

    mov word [kernel_size], 15 * 512
    ret

disk_error:
    mov si, disk_error_msg
    call print_string
    jmp $

load_error:
    mov si, load_error_msg
    call print_string
    jmp $

print_string:
    lodsb
    or al, al
    jz done
    mov ah, 0x0E
    int 0x10
    jmp print_string

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

; === Data ===
boot_msg db "PS2 x86 Bootloader v3.9", 0x0D, 0x0A, 0
kernel_loaded_msg db "Kernel loaded, transferring control. Waiting on bootable device...", 0x0D, 0x0A, 0
load_error_msg db "Error loading kernel!", 0x0D, 0x0A, 0
disk_error_msg db "Disk read error!", 0x0D, 0x0A, 0
not_elf_msg db "Invalid ELF file!", 0x0D, 0x0A, 0
boot_drive db 0
kernel_size dw 0

; === Boot sector padding & signature ===
times 510 - ($ - $$) db 0
dw 0xAA55
