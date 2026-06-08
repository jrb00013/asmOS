[BITS 16]
[ORG 0x7C00]

jmp start
nop

OEMLabel        db 'PS2OS   '
BytesPerSector  dw 512
SectorsPerCluster db 1
ReservedSectors dw 17
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
VolumeLabel     db 'PS2OS     '
FileSystemType  db 'FAT12   '

global start
global boot_drive
global disk_error
global print_string

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

    call fatload16_load_kernel

    mov si, kernel_loaded_msg
    call print_string
    call switch_to_pm

disk_error:
    mov si, disk_error_msg
    call print_string
    jmp $

print_string:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string
.done:
    ret

boot_msg db "ASMOS Boot v3.0", 13, 10, 0
kernel_loaded_msg db "Kernel loaded, entering protected mode...", 13, 10, 0
disk_error_msg db "Disk error!", 13, 10, 0

boot_drive db 0

times 510 - ($ - $$) db 0
dw 0xAA55
