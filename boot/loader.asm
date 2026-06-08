; ASMOS stage2 loader at 0x7E00 — FAT12 KERNEL.BIN + protected mode
[BITS 16]
[ORG 0x7E00]

%define FAT_DATA 65
%define FAT_ROOT 51
%define FAT_START 33
%define FAT_SEG 0x1000
%define ROOT_SEG 0x1100
%define BPB_DRIVE 0x7C24

loader_entry:
    mov dl, [BPB_DRIVE]
    mov si, boot_msg
    call print_string
    call fatload_kernel
    mov si, ok_msg
    call print_string
    call switch_to_pm

kernel_name db 'KERNEL  BIN'

fatload_kernel:
    pusha
    mov ax, FAT_SEG
    mov es, ax
    xor bx, bx
    mov eax, FAT_START
    mov ecx, 9
    call read_lba
    mov ax, ROOT_SEG
    mov es, ax
    xor bx, bx
    mov eax, FAT_ROOT
    mov ecx, 14
    call read_lba

    mov ax, ROOT_SEG
    mov ds, ax
    xor di, di
    mov cx, 224
.find:
    push cx
    push di
    mov si, kernel_name
    mov cx, 11
    repe cmpsb
    pop di
    pop cx
    jz .got
    add di, 32
    loop .find
    jmp disk_err
.got:
    mov ax, [di + 26]
    mov [cluster], ax

    mov ax, 0xFFFF
    mov es, ax
    mov bx, 0x0010

    mov ax, FAT_SEG
    mov ds, ax

.cloop:
    mov ax, [cluster]
    cmp ax, 0xFF8
    jae .done

    mov si, ax
    mov cx, si
    dec cx
    dec cx
    mov ax, FAT_DATA
    add ax, cx
    movzx eax, ax
    call read_es

    add bx, 512
    jnc .nc
    push ax
    mov ax, es
    add ax, 0x1000
    mov es, ax
    pop ax
.nc:
    mov ax, [cluster]
    mov si, ax
    shr si, 1
    add si, ax
    mov ax, [si]
    test word [cluster], 1
    jz .nx
    shr ax, 4
    jmp .st
.nx:
    and ax, 0x0FFF
.st:
    mov [cluster], ax
    jmp .cloop
.done:
    xor ax, ax
    mov ds, ax
    popa
    ret

read_lba:
.read_loop:
    push eax
    push ecx
    push bx
    call lba_chs
    mov ah, 0x02
    mov al, 1
    mov dl, [BPB_DRIVE]
    int 0x13
    jc disk_err
    pop bx
    add bx, 512
    pop ecx
    pop eax
    inc eax
    loop .read_loop
    ret

read_es:
    ; eax = LBA, es:bx = buffer
    push eax
    call lba_chs
    mov ah, 0x02
    mov al, 1
    mov dl, [BPB_DRIVE]
    int 0x13
    jc disk_err
    pop eax
    ret

lba_chs:
    ; eax = LBA in, cl/dh/ch out; preserves bx
    push bx
    xor edx, edx
    mov bx, 18
    div bx
    inc dx
    mov cl, dl
    xor edx, edx
    mov bx, 2
    div bx
    mov dh, dl
    mov ch, al
    pop bx
    ret

align 8
gdt:
    dq 0
    dw 0xFFFF, 0, 0x9A00, 0x00CF
    dw 0xFFFF, 0, 0x9200, 0x00CF
gdt_end:
gdtr:
    dw gdt_end - gdt - 1
    dd gdt
CODE_SEG equ 0x08
DATA_SEG equ 0x10

switch_to_pm:
    cli
    lgdt [gdtr]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp CODE_SEG:pm32

[BITS 32]
pm32:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x0009F000
    jmp CODE_SEG:0x100000

[BITS 16]
disk_err:
    mov si, err_msg
    call print_string
    jmp $

print_string:
    lodsb
    or al, al
    jz .x
    mov ah, 0x0E
    int 0x10
    jmp print_string
.x:
    ret

boot_msg db 'ASMOS loader', 13, 10, 0
ok_msg db 'PM...', 13, 10, 0
err_msg db 'ERR', 13, 10, 0
cluster dw 0
