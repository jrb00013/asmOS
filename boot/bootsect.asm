; ASMOS boot sector — 512 bytes, jumps to loader at 0x7E00 (sector 1)
[BITS 16]
[ORG 0x7C00]

jmp short stage2
nop

OEMLabel        db 'PS2OS   '
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
VolumeLabel     db 'PS2OS     '
FileSystemType  db 'FAT12   '

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [DriveNumber], dl
    mov ax, 0x0003
    int 0x10
stage2:
    jmp 0x0000:0x7E00

times 510 - ($ - $$) db 0
dw 0xAA55
