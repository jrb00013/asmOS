; ASMOS stage1 — BPB at 0x7C00, loader at 0x7E00, padded to 17 sectors
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
    jmp stage2_entry

times 510 - ($ - $$) db 0
dw 0xAA55

times 0x7E00 - ($ - $$) db 0

stage2_entry:
    call fatload16_load_kernel
    mov si, kernel_loaded_msg
    call print_string
    call switch_to_pm

%define FAT12_DATA_START 49
%define FAT12_ROOT_LBA 35
%define FAT12_FAT_LBA 17

kernel_name db 'KERNEL  BIN'

fatload16_load_kernel:
    pusha
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov eax, FAT12_FAT_LBA
    mov ecx, 9
    call read_lba
    mov ax, 0x1100
    mov es, ax
    xor bx, bx
    mov eax, FAT12_ROOT_LBA
    mov ecx, 14
    call read_lba
    mov di, 0x1100
    mov cx, 224
.search:
    push cx
    push di
    mov si, kernel_name
    mov cx, 11
    repe cmpsb
    pop di
    pop cx
    jz .found
    add di, 32
    loop .search
    jmp disk_error
.found:
    mov ax, [di + 26]
    mov [load_cluster], ax
    mov ax, 0xFFFF
    mov es, ax
    mov bx, 0x0010
    mov ax, [load_cluster]
.cluster_loop:
    cmp ax, 0xFF8
    jae .done
    push ax
    mov dx, ax
    mov si, dx
    shr si, 1
    add si, dx
    mov si, [0x1000 + si]
    test ax, 1
    jz .even_fat
    shr si, 4
    jmp .read_cluster
.even_fat:
    and si, 0x0FFF
.read_cluster:
    pop ax
    push ax
    mov eax, FAT12_DATA_START
    movzx esi, si
    sub esi, 2
    add eax, esi
    mov ecx, 1
    call read_lba_current_es
    add bx, 512
    jnc .no_carry
    mov ax, es
    add ax, 0x1000
    mov es, ax
.no_carry:
    mov dx, ax
    mov si, dx
    shr si, 1
    add si, dx
    mov ax, [0x1000 + si]
    test word [load_cluster], 1
    jz .next_even
    shr ax, 4
    jmp .store_next
.next_even:
    and ax, 0x0FFF
.store_next:
    mov [load_cluster], ax
    mov ax, [load_cluster]
    jmp .cluster_loop
.done:
    popa
    ret

read_lba:
    push eax
.rl_loop:
    push eax
    push cx
    push bx
    call lba_to_chs
    mov ah, 0x02
    mov al, 1
    mov dl, [boot_drive]
    int 0x13
    jc disk_error
    pop bx
    add bx, 512
    pop cx
    pop eax
    inc eax
    loop .rl_loop
    pop eax
    ret

read_lba_current_es:
    push eax
    call lba_to_chs
    mov ah, 0x02
    mov al, 1
    mov dl, [boot_drive]
    int 0x13
    jc disk_error
    pop eax
    ret

lba_to_chs:
    xor edx, edx
    mov di, 18
    div di
    inc dx
    mov cl, dl
    xor edx, edx
    mov di, 2
    div di
    mov dh, dl
    mov ch, al
    ret

align 8
gdt_start:
    dq 0
    dw 0xFFFF, 0x0000, 0x9A00, 0x00CF
    dw 0xFFFF, 0x0000, 0x9200, 0x00CF
gdt_end:
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
CODE_SEG equ 0x08
DATA_SEG equ 0x10

switch_to_pm:
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SEG:init_pm32

[BITS 32]
init_pm32:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x0009F000
    jmp CODE_SEG:0x100000

[BITS 16]
disk_error:
    mov si, disk_error_msg
    call print_string
    jmp $

print_string:
    lodsb
    or al, al
    jz .ps_done
    mov ah, 0x0E
    int 0x10
    jmp print_string
.ps_done:
    ret

boot_msg db "ASMOS Boot v3.0", 13, 10, 0
kernel_loaded_msg db "Kernel loaded, entering protected mode...", 13, 10, 0
disk_error_msg db "Disk error!", 13, 10, 0
boot_drive db 0
load_cluster dw 0

times (17 * 512) - ($ - $$) db 0
