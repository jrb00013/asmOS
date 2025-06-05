[BITS 32]
section .text

; Exported functions
global load_kernel_from_disk
global init_fat12
global fat12_list_files
global disable_interrupts_asm
global enable_interrupts_asm

; Imported functions
extern print_string

; Constants
KERNEL_LOAD_SEGMENT equ 0x1000

; Data section
section .data
kernel_filename db "KERNEL  BIN",0
disk_error_msg db "Disk error!",0
read_error_msg db "Read error!",0
file_not_found_msg db "Kernel not found!",0
sectors_per_track dd 18
heads_per_cylinder dd 2
current_cluster dd 0
kernel_size dd 0

; Uninitialized data
section .bss
buffer resb 512*14  ; Enough for root directory

; Code section
section .text

disable_interrupts_asm:
    cli
    ret

enable_interrupts_asm:
    sti
    ret

init_fat12:
    ret

load_kernel_from_disk:
    pusha
    
    ; Reset disk system
    mov eax, 0x00
    mov edx, 0x80          ; First hard disk
    int 0x13
    jc .disk_error
    
    ; Load root directory (LBA 19, 14 sectors)
    mov eax, 19             ; First sector of root directory
    mov ebx, buffer
    mov ecx, 14             ; Number of sectors to read
    call read_sectors
    
    ; Search for kernel file
    mov edi, buffer
    mov ecx, 224            ; Max root directory entries
    
.search_loop:
    push ecx
    push edi
    mov ecx, 11             ; Filename length
    mov esi, kernel_filename
    repe cmpsb
    pop edi
    pop ecx
    je .found_kernel
    
    add edi, 32             ; Next directory entry
    loop .search_loop
    
    jmp .file_not_found
    
.found_kernel:
    ; Load FAT (LBA 1, 9 sectors)
    mov eax, 1              ; FAT starts at sector 1
    mov ebx, buffer
    mov ecx, 9              ; Sectors per FAT
    call read_sectors
    
    ; Load kernel file
    movzx eax, word [edi + 26]  ; First cluster
    mov ebx, KERNEL_LOAD_SEGMENT
    mov es, ebx
    xor ebx, ebx            ; Offset 0
    
.load_cluster:
    ; Convert cluster to LBA
    push eax
    add eax, 31             ; Data sector = cluster + 31
    
    ; Read sector
    mov ecx, 1
    call read_sectors
    
    ; Get next cluster
    pop eax
    call get_next_cluster
    cmp eax, 0xFF8          ; End of file?
    jb .load_cluster
    
    ; Store kernel size
    mov [kernel_size], ebx
    
    popa
    ret
    
.disk_error:
    mov esi, disk_error_msg
    call print_string
    jmp $
    
.file_not_found:
    mov esi, file_not_found_msg
    call print_string
    jmp $

read_sectors:
    ; eax = LBA, ebx = buffer, ecx = sector count
    pusha
    mov edi, 5              ; Retry count
    
.retry:
    push eax
    push ecx
    push ebx
    
    ; Convert LBA to CHS
    xor edx, edx
    div dword [sectors_per_track]
    inc edx
    mov cl, dl              ; Sector
    xor edx, edx
    div dword [heads_per_cylinder]
    mov dh, dl              ; Head
    mov ch, al              ; Cylinder
    
    ; BIOS read sectors
    mov ah, 0x02
    mov al, 1               ; Sectors to read
    pop ebx
    pop ecx
    push ecx
    push ebx
    mov dl, 0x80            ; Drive number
    
    int 0x13
    jnc .success
    
    ; Error occurred
    dec edi
    jz .read_error
    
    ; Reset disk and retry
    xor ah, ah
    int 0x13
    pop ebx
    pop ecx
    pop eax
    jmp .retry
    
.success:
    pop ebx
    pop ecx
    pop eax
    
    ; Update buffer and LBA
    add ebx, 512
    inc eax
    loop read_sectors
    
    popa
    ret
    
.read_error:
    mov esi, read_error_msg
    call print_string
    jmp $

get_next_cluster:
    ; eax = current cluster, returns next cluster in eax
    push ebx
    push ecx
    
    ; Calculate FAT offset
    mov ebx, eax
    shr ebx, 1              ; Divide by 2
    add ebx, eax            ; Multiply by 3/2
    
    mov ax, [buffer + ebx]
    
    test dword [current_cluster], 1
    jz .even_cluster
    
.odd_cluster:
    shr ax, 4
    jmp .done
    
.even_cluster:
    and ax, 0x0FFF
    
.done:
    movzx eax, ax
    pop ecx
    pop ebx
    ret

fat12_list_files:
    ; Implement your file listing functionality
    ret