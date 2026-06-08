; ASMOS stage2 loader at 0x7E00 — FAT12 KERNEL.BIN + protected mode
[BITS 16]
[ORG 0x7E00]

%define FAT_DATA 33
%define FAT_ROOT 19
%define FAT_START 1

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
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov eax, FAT_START
    mov ecx, 9
    call read_lba
    mov ax, 0x1100
    mov es, ax
    xor bx, bx
    mov eax, FAT_ROOT
    mov ecx, 14
    call read_lba
    mov di, 0x1100
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
    mov ax, [cluster]
.cloop:
    cmp ax, 0xFF8
    jae .done
    push ax
    mov si, ax
    shr si, 1
    add si, ax
    mov si, [0x1000 + si]
    test ax, 1
    jz .ev
    shr si, 4
    jmp .rd
.ev:
    and si, 0x0FFF
.rd:
    pop ax
    push ax
    mov eax, FAT_DATA
    movzx esi, si
    sub esi, 2
    add eax, esi
    mov ecx, 1
    call read_es
    add bx, 512
    jnc .nc
    mov ax, es
    add ax, 0x1000
    mov es, ax
.nc:
    push ax
    mov bx, ax
    shr bx, 1
    add bx, ax
    mov ax, [0x1000 + bx]
    pop bx
    test bx, 1
    jz .nx
    shr ax, 4
    jmp .st
.nx:
    and ax, 0x0FFF
.st:
    mov [cluster], ax
    mov ax, [cluster]
    jmp .cloop
.done:
    popa
    ret

read_lba:
    push eax
.rl:
    push eax
    push cx
    push bx
    call lba_chs
    mov ah, 0x02
    mov al, 1
    mov dl, [BPB_DRIVE]
    int 0x13
    jc disk_err
    pop bx
    add bx, 512
    pop cx
    pop eax
    inc eax
    loop .rl
    pop eax
    ret

read_es:
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
    xor dx, dx
    mov di, 18
    div di
    inc dx
    mov cl, dl
    xor dx, dx
    mov di, 2
    div di
    mov dh, dl
    mov ch, al
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
