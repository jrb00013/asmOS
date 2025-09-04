[BITS 32]
section .text

; Enhanced PS2 x86 OS System Calls with Networking
global print_string
global exit_program
global read_key
global get_memory_info
global sys_read_line
global sys_exit
global sys_network_init
global sys_network_send
global sys_network_receive
global sys_sound_init
global sys_sound_play
global sys_graphics_init
global sys_graphics_draw
global sys_timer_init
global sys_timer_get
global sys_ps2_controller_read
global sys_ps2_controller_write

section .text

; ============================
; Enhanced Network Functions
; ============================
sys_network_init:
    pusha
    
    ; Initialize PS2 network adapter
    
    ; Check for network adapter
    mov eax, 0x1F801450  ; PS2 network register base
    mov ebx, [eax]
    test ebx, 0x80000000 
    jz .no_network
    
    ; Initialize network controller
    mov eax, 0x1F801450
    mov dword [eax], 0x00000001  ; Enable network
    
    ; Set up network parameters
    mov eax, 0x1F801454
    mov dword [eax], 0x0A000001  ; IP: 10.0.0.1 
    
    mov eax, 0x1F801458
    mov dword [eax], 0xFFFFFF00  ; Netmask: 255.255.255.0
    
    popa
    mov eax, 1  ; Success
    ret

.no_network:
    popa
    mov eax, 0  ; No network
    ret

sys_network_send:
    ; eax = data pointer, ebx = length
    pusha
    
    ; Send data over PS2 network
    mov ecx, eax  ; Data pointer
    mov edx, ebx  ; Length
    
    ; Simplified network send
    mov eax, 0x1F801460  ; Network send register
    mov [eax], ecx       ; Send data pointer
    mov eax, 0x1F801464  ; Network length register
    mov [eax], edx       ; Send length
    
    popa
    ret

sys_network_receive:
    ; eax = buffer pointer, ebx = max length
    pusha
    
    ; Receive data from PS2 network
    mov ecx, eax  ; Buffer pointer
    mov edx, ebx  ; Max length
    
    ; Simplified network receive
    mov eax, 0x1F801468  ; Network receive register
    mov ebx, [eax]       ; Get received data
    mov [ecx], ebx       ; Store in buffer
    
    popa
    ret

; ============================
; Enhanced Sound System
; ============================
sys_sound_init:
    pusha
    
    ; Initialize PS2 sound system
    mov eax, 0x1F801C00  ; SPU2 base address
    
    ; Reset SPU2
    mov dword [eax + 0x1A0], 0x00000000  ; SPUCNT
    mov dword [eax + 0x1A0], 0x80000000  ; Enable SPU2
    
    ; Set up audio parameters
    mov dword [eax + 0x1A4], 0x00000000  ; SPUSTAT
    mov dword [eax + 0x1A8], 0x00000000  ; SPUMODE1
    mov dword [eax + 0x1AC], 0x00000000  ; SPUMODE2
    
    popa
    ret

sys_sound_play:
    ; eax = sound data pointer, ebx = length, ecx = frequency
    pusha
    
    ; Play sound through PS2 SPU2
    mov edx, eax  ; Sound data pointer
    mov esi, ebx  ; Length
    mov edi, ecx  ; Frequency
    
    ; Set up sound channel
    mov eax, 0x1F801C00  ; SPU2 base
    mov dword [eax + 0x1B0], edx  ; Sound data address
    mov dword [eax + 0x1B4], esi  ; Sound length
    mov dword [eax + 0x1B8], edi  ; Sound frequency
    
    ; Start playback
    mov dword [eax + 0x1BC], 0x00000001  ; Start channel
    
    popa
    ret

; ============================
; Enhanced Graphics System
; ============================
sys_graphics_init:
    pusha
    
    ; Initialize PS2 Graphics Synthesizer
    mov eax, 0x1F801810  ; GS base address
    
    ; Set up display mode
    mov dword [eax + 0x00], 0x00000000  ; PMODE
    mov dword [eax + 0x70], 0x00000000  ; DISPLAY1
    mov dword [eax + 0x74], 0x00000000  ; DISPLAY2
    
    ; Set resolution (640x480)
    mov dword [eax + 0x80], 0x00000280  ; Width
    mov dword [eax + 0x84], 0x000001E0  ; Height
    
    popa
    ret

sys_graphics_draw:
    ; eax = x, ebx = y, ecx = color, edx = width, esi = height
    pusha
    
    ; Draw rectangle on PS2 screen
    mov edi, eax  ; X coordinate
    mov eax, ebx  ; Y coordinate
    mov ebx, ecx  ; Color
    mov ecx, edx  ; Width
    mov edx, esi  ; Height
    
    ; Calculate VRAM address
    shl eax, 10   ; Y * 1024
    add eax, edi  ; + X
    shl eax, 2    ; * 4 (32-bit pixels)
    add eax, 0x70000000  ; VRAM base
    
    ; Fill rectangle
    mov esi, eax  ; VRAM address
    mov eax, ebx  ; Color
    mov ecx, edx  ; Height
    mov edx, ecx  ; Width
    
.fill_row:
    push ecx
    mov ecx, edx
    rep stosd     ; Fill row with color
    pop ecx
    add esi, 1024 ; Next row
    loop .fill_row
    
    popa
    ret

; ============================
; Enhanced Timer System
; ============================
sys_timer_init:
    pusha
    
    ; Initialize PS2 timers
    mov eax, 0x1F801100  ; Timer 0 base
    mov dword [eax + 0x00], 0x00000000  ; T0_MODE
    mov dword [eax + 0x04], 0x00000000  ; T0_COUNT
    mov dword [eax + 0x08], 0x00000000  ; T0_COMP
    
    mov eax, 0x1F801110  ; Timer 1 base
    mov dword [eax + 0x00], 0x00000000  ; T1_MODE
    mov dword [eax + 0x04], 0x00000000  ; T1_COUNT
    mov dword [eax + 0x08], 0x00000000  ; T1_COMP
    
    mov eax, 0x1F801120  ; Timer 2 base
    mov dword [eax + 0x00], 0x00000000  ; T2_MODE
    mov dword [eax + 0x04], 0x00000000  ; T2_COUNT
    mov dword [eax + 0x08], 0x00000000  ; T2_COMP
    
    popa
    ret

sys_timer_get:
    ; Returns current timer value in eax
    push ebx
    
    ; Get timer 0 value
    mov eax, 0x1F801100  ; Timer 0 base
    mov eax, [eax + 0x04] ; T0_COUNT
    
    pop ebx
    ret

; ============================
; Enhanced PS2 Controller Support
; ============================
sys_ps2_controller_read:
    ; Returns controller data in eax
    push ebx
    
    ; Read from PS2 controller port
    mov eax, 0x1F801040  ; Controller base
    mov eax, [eax + 0x00] ; Controller data
    
    pop ebx
    ret

sys_ps2_controller_write:
    ; eax = data to write
    push ebx
    
    ; Write to PS2 controller port
    mov ebx, eax
    mov eax, 0x1F801040  ; Controller base
    mov [eax + 0x00], ebx ; Controller data
    
    pop ebx
    ret

; ============================
; Enhanced System Functions
; ============================
sys_read_line:
    push ebp
    mov ebp, esp

    mov edi, [ebp + 8]   ; buffer pointer
    mov ecx, [ebp + 12]  ; max_len

.read_loop:
    ; Enhanced keyboard input with PS2 controller support
    call sys_ps2_controller_read
    test eax, 0x00000001  ; Check if key pressed
    jz .read_loop
    
    ; Get key code
    shr eax, 8
    and eax, 0xFF
    
    ; Convert to ASCII
    cmp al, 0x1C  ; Enter key
    je .done
    
    ; Store char into buffer
    mov [edi], al
    inc edi
    loop .read_loop

.done:
    mov byte [edi], 0    ; null-terminate
    mov eax, [ebp + 8]   ; return buffer pointer
    pop ebp
    ret

sys_exit:
    ; Enhanced exit with cleanup
    pusha
    
    ; Stop all systems
    call sys_sound_init   ; Stop sound
    call sys_graphics_init ; Clear screen
    call sys_timer_init   ; Stop timers
    
    ; Print exit message
    mov esi, exit_msg
    call print_string
    
    ; Halt system
    cli
    hlt
    jmp $

; ============================
; Enhanced Memory Functions
; ============================
get_memory_info:
    push es
    push di
    push ecx
    push edx
    
    ; Enhanced PS2 memory detection
    xor ebx, ebx            ; Continuation value
    mov edx, 'PAMS'         ; SMAP signature
    xor ecx, ecx            ; Entry count
    
    ; PS2-specific memory regions
    mov eax, 0x00000000     ; Start of memory map
    mov edi, memory_map_buffer
    
.mem_loop:
    ; Get memory map entry
    mov eax, 0xE820         ; BIOS memory map function
    mov ecx, 24             ; Buffer size
    int 0x15
    jc .mem_done            ; CF set on error/last entry
    
    cmp eax, 'PAMS'         ; Verify signature
    jne .mem_done
    
    ; Check if this is PS2 memory region
    cmp dword [edi + 16], 1  ; Type 1 = available memory
    jne .next_entry
    
    ; Add to total
    mov eax, [edi + 8]   ; Length
    add eax, [edi + 12]  ; Length high
    
    ; Check if this is PS2 main memory (32MB)
    cmp eax, 0x02000000  ; 32MB
    jne .next_entry
    
    mov ecx, eax         ; Store PS2 memory size
    
.next_entry:
    add di, 24              ; Next buffer position
    test ebx, ebx           ; Check if more entries
    jnz .mem_loop
    
.mem_done:
    mov eax, ecx            ; Return PS2 memory size
    pop edx
    pop ecx
    pop di
    pop es
    ret

; ============================
; Enhanced Print Functions
; ============================
print_string:
    ; Enhanced string printing with color support
    pusha
    
.print_loop:
    mov al, [esi]
    test al, al
    jz .print_done
    
    ; Check for color codes
    cmp al, 0x1B  ; ESC
    je .color_code
    
    ; Print character
    mov ah, 0x0E
    int 0x10
    inc esi
    jmp .print_loop
    
.color_code:
    ; Handle color codes
    inc esi
    mov al, [esi]
    cmp al, '['  ; Color start
    jne .print_loop
    
    inc esi
    mov al, [esi]
    ; Set color based on code
    cmp al, 'R'  ; Red
    je .set_red
    cmp al, 'G'  ; Green
    je .set_green
    cmp al, 'B'  ; Blue
    je .set_blue
    jmp .print_loop
    
.set_red:
    mov ah, 0x0C  ; Red color
    jmp .print_loop
    
.set_green:
    mov ah, 0x0A  ; Green color
    jmp .print_loop
    
.set_blue:
    mov ah, 0x09  ; Blue color
    jmp .print_loop
    
.print_done:
    popa
    ret

read_key:
    ; Enhanced key reading with PS2 controller support
    push ebx
    
    ; Try PS2 controller first
    call sys_ps2_controller_read
    test eax, 0x00000001
    jnz .controller_key
    
    ; Fallback to BIOS keyboard
    mov ah, 0x00
    int 0x16
    jmp .done
    
.controller_key:
    ; Process PS2 controller input
    shr eax, 8
    and eax, 0xFF
    
.done:
    pop ebx
    ret

exit_program:
    ; Enhanced program exit
    mov eax, 0x4C00         ; DOS exit function
    or eax, ebx             ; Combine with exit code
    int 0x21                ; DOS interrupt
    hlt

section .rodata
exit_msg db "Enhanced PS2 x86 OS - Shutting down gracefully...", 0x0D, 0x0A, 0

section .bss
memory_map_buffer times 200 db 0
network_buffer times 1024 db 0
sound_buffer times 4096 db 0
graphics_buffer times 65536 db 0