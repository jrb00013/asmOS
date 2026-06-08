; 16-bit real-mode FAT12 kernel loader
[BITS 16]

%define FAT12_DATA_START 49
%define FAT12_ROOT_LBA 35
%define FAT12_FAT_LBA 17

section .text

global fatload16_load_kernel

kernel_name db 'KERNEL  BIN'

fatload16_load_kernel:
    pusha

    ; Load FAT (9 sectors) to 0x10000
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov eax, FAT12_FAT_LBA
    mov ecx, 9
    call read_lba

    ; Load root (14 sectors) to 0x11000
    mov ax, 0x1100
    mov es, ax
    xor bx, bx
    mov eax, FAT12_ROOT_LBA
    mov ecx, 14
    call read_lba

    ; Find KERNEL.BIN
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

    ; Destination 0x100000 => ES:BX = FFFF:0010
    mov ax, 0xFFFF
    mov es, ax
    mov bx, 0x0010
    mov ax, [load_cluster]

.cluster_loop:
    cmp ax, 0xFF8
    jae .done

    push ax
    mov dx, ax
    mov bx, dx
    shr bx, 1
    add bx, dx
    mov si, [0x1000 + bx]
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
    mov bx, dx
    shr bx, 1
    add bx, dx
    mov ax, [0x1000 + bx]
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

section .data
load_cluster dw 0
