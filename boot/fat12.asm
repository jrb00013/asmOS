[BITS 16]

; Load kernel from FAT12 filesystem
load_kernel:
    pusha
    
      
    mov ah, 0x00
    mov dl, 0x80            ; First hard disk
    int 0x13
    jc .disk_error
    
    ; Load root directory
    mov ax, 19              ; First sector of root directory
    mov bx, buffer
    mov cx, 14              ; Number of sectors to read
    call read_sectors
    
    ; Search for kernel file
    mov di, buffer
    mov cx, 224             ; Max root directory entries
    mov si, kernel_filename
    
.search_loop:
    push cx
    push di
    mov cx, 11             ; Filename length in FAT12
    repe cmpsb
    pop di
    pop cx
    je .found_kernel
    
    add di, 32             ; Next directory entry
    loop .search_loop
    
    jmp .file_not_found
    
.found_kernel:
    ; Load FAT
    mov ax, 1               ; FAT starts at sector 1
    mov bx, buffer
    mov cx, 9               ; Sectors per FAT
    call read_sectors
    
    ; Load kernel file
    mov ax, word [di + 26]  ; First cluster
    mov bx, 0x1000          ; Load to segment 0x1000
    mov es, bx
    xor bx, bx              ; Offset 0
    
.load_cluster:
    ; Convert cluster to LBA
    push ax
    add ax, 31              ; Data sector = cluster + 31
    
    ; Read sector
    mov cx, 1
    call read_sectors
    
    ; Get next cluster
    pop ax
    call get_next_cluster
    cmp ax, 0xFF8           ; End of file?
    jb .load_cluster
    
    ; Store kernel size
    mov [kernel_size], bx
    
    popa
    ret
    
.disk_error:
    mov si, disk_error_msg
    call print_string
    jmp $
    
.file_not_found:
    mov si, file_not_found_msg
    call print_string
    jmp $

; Helper functions
read_sectors:
    ; AX = LBA, ES:BX = buffer, CX = sector count
    pusha
    mov di, 5               ; Retry count
    
.retry:
    push ax
    push cx
    push bx
    
    ; Convert LBA to CHS
    xor dx, dx
    div word [sectors_per_track]
    inc dl
    mov cl, dl              ; Sector
    xor dx, dx
    div word [heads_per_cylinder]
    mov dh, dl              ; Head
    mov ch, al              ; Cylinder
    
    ; BIOS read sectors
    mov ah, 0x02
    mov al, 1               ; Sectors to read
    pop bx
    pop cx
    push cx
    push bx
    mov dl, 0x80            ; Drive number
    
    int 0x13
    jnc .success
    
    ; Error occurred
    dec di
    jz .read_error
    
    ; Reset disk and retry
    xor ah, ah
    int 0x13
    pop bx
    pop cx
    pop ax
    jmp .retry
    
.success:
    pop bx
    pop cx
    pop ax
    
    ; Update buffer and LBA
    add bx, 512
    inc ax
    loop read_sectors
    
    popa
    ret
    
.read_error:
    mov si, read_error_msg
    call print_string
    jmp $

get_next_cluster:
    ; AX = current cluster, returns next cluster in AX
    push bx
    push cx
    
    ; Calculate FAT offset
    mov bx, ax
    shr bx, 1               ; Divide by 2
    add bx, ax              ; Multiply by 3/2
    
    mov ax, [buffer + bx]
    
    test word [current_cluster], 1
    jz .even_cluster
    
.odd_cluster:
    shr ax, 4
    jmp .done
    
.even_cluster:
    and ax, 0x0FFF
    
.done:
    pop cx
    pop bx
    ret

; Data
kernel_filename db "KERNEL  BIN"
disk_error_msg db "Disk error!", 0x0D, 0x0A, 0
read_error_msg db "Read error!", 0x0D, 0x0A, 0
file_not_found_msg db "Kernel not found!", 0x0D, 0x0A, 0
sectors_per_track dw 18
heads_per_cylinder dw 2
current_cluster dw 0
buffer:                     ; Disk read buffer
