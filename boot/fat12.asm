[BITS 32]
section .text

; Exported functions
global load_kernel_from_disk
global init_fat12
global fat12_list_files
global disable_interrupts_asm
global enable_interrupts_asm
global detect_ps2_memory
global init_ps2_controllers

; Imported functions
extern print_string

; Constants
KERNEL_LOAD_SEGMENT equ 0x1000
PS2_MEMORY_BASE equ 0x20000000  ; PS2 memory starts at 512MB

; Data section
section .data
kernel_filename db "KERNEL  BIN",0
disk_error_msg db "Disk error!",0
read_error_msg db "Read error!",0
file_not_found_msg db "Kernel not found!",0
ps2_memory_msg db "PS2 memory detected: ",0
sectors_per_track dd 18
heads_per_cylinder dd 2
current_cluster dd 0
kernel_size dd 0

; Enhanced file listing
file_count dd 0
total_size dd 0

; Uninitialized data
section .bss
buffer resb 512*32  ; Increased buffer for larger files
fat_buffer resb 512*9  ; FAT table buffer
root_buffer resb 512*14  ; Root directory buffer

; Code section
section .text

disable_interrupts_asm:
    cli
    ret

enable_interrupts_asm:
    sti
    ret

; PS2 memory detection
detect_ps2_memory:
    push ebx
    push ecx
    push edx
    push edi
    
    ; Try to get memory map using BIOS E820
    xor ebx, ebx
    mov edx, 'PAMS'
    mov ecx, 20
    mov edi, memory_map_buffer
    
    mov eax, 0xE820
    int 0x15
    jc .fallback_memory
    
    ; If we got memory map, calculate total
    mov eax, 0
    mov ecx, 0
    
.memory_loop:
    cmp dword [edi + 16], 1  ; Type 1 = available memory
    jne .next_entry
    
    mov eax, [edi + 8]   ; Length
    add eax, [edi + 12]  ; Length high
    add ecx, eax
    
.next_entry:
    add edi, 20
    test ebx, ebx
    jnz .memory_loop
    
    ; Convert to MB
    shr ecx, 20
    mov eax, ecx
    jmp .done
    
.fallback_memory:
    ; Fallback: assume PS2 has 32MB
    mov eax, 32
    
.done:
    pop edi
    pop edx
    pop ecx
    pop ebx
    ret

; PS2 controller initialization
init_ps2_controllers:
    pusha
    
    ; Initialize PS2 controller ports
    ; This is a simplified version - real PS2 controller init is complex
    
    ; Reset controller 1
    mov al, 0xAD  ; Disable first PS2 port
    out 0x64, al
    mov al, 0xAE  ; Enable first PS2 port
    out 0x64, al
    
    ; Reset controller 2 (if present)
    mov al, 0xA7  ; Disable second PS2 port
    out 0x64, al
    mov al, 0xA8  ; Enable second PS2 port
    out 0x64, al
    
    popa
    ret

init_fat12:
    ; Initialize FAT12 filesystem
    ; Load FAT table into memory
    mov eax, 1              ; FAT starts at sector 1
    mov ebx, fat_buffer
    mov ecx, 9              ; Sectors per FAT
    call read_sectors
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
    mov ebx, root_buffer
    mov ecx, 14             ; Number of sectors to read
    call read_sectors
    
    ; Search for kernel file
    mov edi, root_buffer
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

; Enhanced file listing
fat12_list_files:
    pusha
    
    ; Load root directory
    mov eax, 19
    mov ebx, root_buffer
    mov ecx, 14
    call read_sectors
    
    ; Search for files
    mov edi, root_buffer
    mov ecx, 224
    mov dword [file_count], 0
    mov dword [total_size], 0
    
.list_loop:
    ; Check if entry is a file (not empty, not deleted, not directory)
    mov al, [edi]
    cmp al, 0x00            ; Empty entry
    je .next_entry
    cmp al, 0xE5            ; Deleted file
    je .next_entry
    
    mov al, [edi + 11]      ; Attributes
    test al, 0x10           ; Directory bit
    jnz .next_entry
    
    ; Print filename
    push edi
    mov ecx, 8              ; Name length
    mov esi, edi
.print_name:
    mov al, [esi]
    cmp al, ' '
    je .print_ext
    mov ah, 0x0E
    int 0x10
    inc esi
    loop .print_name
    
.print_ext:
    mov al, '.'
    mov ah, 0x0E
    int 0x10
    
    mov ecx, 3              ; Extension length
    mov esi, edi
    add esi, 8
.print_ext_loop:
    mov al, [esi]
    cmp al, ' '
    je .print_size
    mov ah, 0x0E
    int 0x10
    inc esi
    loop .print_ext_loop
    
.print_size:
    ; Print file size
    mov al, ' '
    mov ah, 0x0E
    int 0x10
    
    mov eax, [edi + 28]     ; File size
    push eax
    call print_decimal
    pop eax
    
    add dword [total_size], eax
    inc dword [file_count]
    
    ; Print newline
    mov al, 0x0D
    mov ah, 0x0E
    int 0x10
    mov al, 0x0A
    mov ah, 0x0E
    int 0x10
    
    pop edi
    
.next_entry:
    add edi, 32
    loop .list_loop
    
    ; Print summary
    mov al, 0x0D
    mov ah, 0x0E
    int 0x10
    mov al, 0x0A
    mov ah, 0x0E
    int 0x10
    
    ; Print file count
    mov eax, [file_count]
    call print_decimal
    mov al, ' '
    mov ah, 0x0E
    int 0x10
    mov al, 'f'
    mov ah, 0x0E
    int 0x10
    mov al, 'i'
    mov ah, 0x0E
    int 0x10
    mov al, 'l'
    mov ah, 0x0E
    int 0x10
    mov al, 'e'
    mov ah, 0x0E
    int 0x10
    mov al, 's'
    mov ah, 0x0E
    int 0x10
    
    ; Print total size
    mov al, ' '
    mov ah, 0x0E
    int 0x10
    mov al, '('
    mov ah, 0x0E
    int 0x10
    mov eax, [total_size]
    call print_decimal
    mov al, ' '
    mov ah, 0x0E
    int 0x10
    mov al, 'b'
    mov ah, 0x0E
    int 0x10
    mov al, 'y'
    mov ah, 0x0E
    int 0x10
    mov al, 't'
    mov ah, 0x0E
    int 0x10
    mov al, 'e'
    mov ah, 0x0E
    int 0x10
    mov al, 's'
    mov ah, 0x0E
    int 0x10
    mov al, ')'
    mov ah, 0x0E
    int 0x10
    
    popa
    ret

; Print decimal number
print_decimal:
    push ebx
    push ecx
    push edx
    
    mov ebx, 10
    mov ecx, 0
    
.div_loop:
    xor edx, edx
    div ebx
    push edx
    inc ecx
    test eax, eax
    jnz .div_loop
    
.print_loop:
    pop eax
    add al, '0'
    mov ah, 0x0E
    int 0x10
    loop .print_loop
    
    pop edx
    pop ecx
    pop ebx
    ret

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
    
    mov ax, [fat_buffer + ebx]
    
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

; Memory map buffer for PS2 detection
memory_map_buffer times 100 db 0