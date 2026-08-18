; ASMOS boot sector — 512 bytes, jumps to loader at 0x7E00 (sector 1)
[BITS 16]
[ORG 0x7C00]

jmp short start
nop

OEMLabel        db 'PS2OS   '
BytesPerSector  dw 512
SectorsPerCluster db 1
ReservedSectors dw 33
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

    ; BIOS int19h boot only ever loads THIS 512-byte boot sector into
    ; 0000:7C00 and jumps here — it does not load anything else. The
    ; stage2 loader lives on disk at LBA 1 (written there by the Makefile's
    ; disk image recipe) but nothing brought it into memory before this
    ; fix, so "jmp 0x0000:0x7E00" used to jump into whatever garbage
    ; happened to be sitting in RAM. Read LBA 1 (CHS: cylinder 0, head 0,
    ; sector 2, using this BPB's own SectorsPerTrack/NumHeads) into
    ; 0000:7E00 first, with a bounded retry + controller reset, matching
    ; the retry pattern loader.asm's own read_lba already uses for later
    ; FAT12 reads.
    mov bp, 3               ; bounded retry count (kept out of cx —
                             ; cx doubles as the CHS cylinder/sector
                             ; fields for int 0x13 below)
.load_loader:
    mov ah, 0x02            ; BIOS: read sectors (CHS)
    mov al, 4                ; sectors — generous headroom above loader.bin's
                             ; current size (matching the 32-sector budget
                             ; the Makefile's stage1 padding already assumes)
    mov ch, 0                ; cylinder 0
    mov cl, 2                ; sector 2 (1-indexed) == LBA 1 for head 0
    mov dh, 0                ; head 0
    mov dl, [DriveNumber]
    xor bx, bx
    mov es, bx
    mov bx, 0x7E00           ; ES:BX = 0000:7E00, matches loader's ORG
    int 0x13
    jnc stage2_ready
    xor ax, ax               ; AH=0: reset disk controller before retrying
    int 0x13
    dec bp
    jnz .load_loader
    mov si, load_err_msg
.err_print:
    lodsb
    or al, al
    jz .halt
    mov ah, 0x0E
    int 0x10
    jmp .err_print
.halt:
    hlt
    jmp .halt

stage2_ready:
    jmp 0x0000:0x7E00

load_err_msg db 'DISK ERR', 13, 10, 0

times 510 - ($ - $$) db 0
dw 0xAA55
