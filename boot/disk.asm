; disk.asm

[bits 16]

read_sector:
    pusha

    ; Inputs:
    ;   - ES:BX = destination memory address
    ;   - DL = drive number (usually 0x00 for floppy or 0x80 for hard disk)
    ;   - CH = cylinder
    ;   - CL = sector (1–63)
    ;   - DH = head
    ;   - AL = number of sectors to read

.retry:
    mov ah, 0x02        ; BIOS read sectors
    int 0x13
    jc .error           ; carry set = error
    popa
    ret

.error:
    ; simple retry mechanism
    mov ah, 0x00        ; reset disk
    int 0x13
    jmp .retry
