; 82077 floppy controller — DMA-mode sector I/O for the boot floppy (drive A:).
;
; Root cause this fixes: platform/x86/hal_storage.c reads sectors through
; fat12_read_sector -> disk_read_sector, which is an ATA PIO driver on the
; primary channel (0x1F0). But disk/os.img is a 1.44MB floppy-formatted
; FAT12 volume attached with -fda; there is no ATA disk behind it. So every
; read returned garbage, plat_fs_init() failed its 0x55AA probe, and `ls`,
; `cat`, `mkdir` all reported failure on a correctly-booted kernel. This is
; the real block device, driven directly so the kernel does not depend on
; the BIOS for I/O once it is in protected mode.
;
; Geometry is not hardcoded: floppy_init() reads sector 0 (always C=0,H=0,S=1
; regardless of geometry) and takes sectors-per-track / head count from the
; FAT12 BPB, so any FAT volume the loader can boot works.
;
; Runs in DMA mode via 8237 channel 2 (the standard FDC data channel). The
; transfer stages through a bounce buffer at 0x00008000 because the 8237
; addresses each channel with a 16-bit offset plus a 4-bit page, so the
; buffer has to sit inside the first 64K and cannot straddle a 64K
; boundary. The kernel itself lives at 0x100000+, where DMA cannot reach at
; all. 0x8000 is clear of everything else: the boot sector ends at 0x7D00,
; the real-mode FAT staging at 0x10000 is dead by protected mode, video
; starts at 0xA0000, and the kernel stack is at 0x9F000 — and 0x8000+512
; stays far below all of them.
;
; Programming style: every routine uses a standard push ebp/mov ebp,esp
; prologue. In-progress per-sector parameters (cylinder/head/sector) live in
; .bss globals rather than being threaded through registers, because the
; kernel is single-threaded and polling — this keeps argument order trivial
; and avoids the stack-offset bugs that hand-passed asm registers invite.
[BITS 32]

section .note.GNU-stack noalloc noexec nowrite progbits
section .text

global floppy_init
global floppy_read_sector
global floppy_write_sector

%define FDC_DATA 0x3F5
%define FDC_MSR  0x3F4        ; main status register (also SRA)
%define FDC_DOR  0x3F2        ; digital output register (also SRB)
%define FDC_DIR  0x3F7        ; digital input register (read-only, disk change)

; DOR bit layout (82077): bits 1-0 drive select, bit 2 nRESET, bit 3 DMA/IRQ
; enable, bits 4-7 motor enables for drives 0-3. Motor-enable for drive 0 is
; 0x10, NOT 0x01 — 0x01 is the low drive-select bit, so writing it selected
; drive 1 and never spun up drive 0's motor.
%define DOR_DRIVE0   0x00     ; drive select 0
%define DOR_MOTOR0   0x10     ; motor enable, drive 0
%define DOR_RESET    0x04     ; 1 = reset released
%define DOR_DMA_IRQ  0x08     ; DMA/IRQ enable
%define DOR_READY    0x80

%define MSR_RQM      0x80     ; request for master (result byte ready)
%define MSR_DIO      0x40     ; 1 = FDC will send data to host (result phase)
%define MSR_BUSY     0x08     ; command in progress

%define CMD_SPECIFY    0x03
%define CMD_SENSE_INT  0x08
%define CMD_RECALIB    0x07
%define CMD_SEEK       0x0F
; Bits 4-0 are the opcode and 7-5 are the flags (MFM, MT, SK); the drive and
; head come from the parameter byte that follows, not from the command byte.
; READ DATA is opcode 0x06 and WRITE DATA is 0x05, so the MFM/MT/SK forms are
; 0xE6 and 0xC5. OR-ing the flags into 0x00 (0xE0/0xC0) yields an undefined
; opcode and the controller reports "unimplemented command".
%define CMD_READ_DATA  0xE6    ; SK + MT + MFM + READ DATA
%define CMD_WRITE_DATA 0xC5    ; MT + MFM + WRITE DATA

; 8237 master DMA, channel 2. Address and count share port 0x04/0x05, but the
; page register is the separate port 0x81 (NOT 0x0A) and 0x0A is the single-
; channel mask register (bits 1-0 select the channel, bit 2 = 1 masks it).
; Writing 0x0A as if it were a page register masked the wrong channel and left
; channel 2 masked, so the FDC's DREQ was never acknowledged and the transfer
; stalled forever with the MSR showing drive-busy.
%define DMA2_ADDR 0x04
%define DMA2_CNT  0x05
%define DMA2_PAGE 0x81        ; channel 2 page register
%define DMA_MASK  0x0A        ; single-channel mask register
%define DMA_MODE  0x0B
%define DMA_FF    0x0C        ; clear the address/count flip-flop
%define DMA_CH2_MASK   0x06   ; channel 2 + mask bit set
%define DMA_CH2_UNMASK 0x02   ; channel 2 + mask bit clear
; Mode register: bits 1-0 channel, bits 3-2 transfer type (00 verify, 01 write,
; 10 read), bits 7-6 single mode. A floppy read moves FDC -> memory = "write"
; (0x46); a floppy write moves memory -> FDC = "read" (0x4A).
%define DMA_M_READ  0x46      ; single mode, FDC -> memory
%define DMA_M_WRITE 0x4A      ; single mode, memory -> FDC

%define FDC_DMA_BUF 0x00008000

section .bss
align 4
fdc_int_pending: resd 1
floppy_spt:   resb 1          ; sectors per track (from BPB)
floppy_heads: resb 1          ; head count (from BPB)
floppy_ready: resb 1
fdc_st0:      resb 1          ; last ST0 (status) byte
io_cyl:       resb 1          ; in-progress sector parameters
io_head:      resb 1
io_sec:       resb 1

section .text

; ---- port / timing helpers ----------------------------------------------

; Read the status register a few times. This both gives the controller a
; moment to latch a data-port write and leaves registers untouched for the
; caller. Preserves eax, ecx, edx.
fdc_settle:
    push eax
    push ecx
    push edx
    mov dx, FDC_MSR
    in al, dx
    mov ecx, 32
.settle:
    in al, dx
    loop .settle
    pop edx
    pop ecx
    pop eax
    ret

; void fdc_send(void) — write the byte in al to the FDC data port.
; The byte travels in al rather than on the stack because the FDC command
; sequences interleave sends with helpers that reuse the argument area.
; The 82077 only latches command bytes while RQM is set; bytes written while it
; is busy are silently dropped, desynchronising the command phase.
fdc_send:
    push eax
    push edx
    push esi
    mov esi, eax
    call fdc_wait_cmd
    test eax, eax
    jz .send_fail
    mov eax, esi
    mov dx, FDC_DATA
    out dx, al
    pop esi
    pop edx
    pop eax
    ret
.send_fail:
    pop esi
    pop edx
    pop eax
    xor eax, eax
    ret

; void fdc_delay_ms(uint32_t ms) — busy-wait roughly `ms` milliseconds using
; the timestamp counter. Iteration-count loops finish in microseconds, which is
; far too fast for the emulated motor spin-up and stepper seeks, both of which
; are driven by a wall-clock timer.
fdc_delay_ms:
    push ebp
    mov ebp, esp
    push eax
    push ecx
    push edx
    push esi
    push ebx
    mov esi, [ebp + 8]
    test esi, esi
    jz .d_out
    rdtsc
    mov ebx, eax
.d_loop:
    rdtsc
    sub eax, ebx
    cmp eax, 100000             ; ~1ms at the calibrated TSC rate
    jb .d_loop
    dec esi
    jnz .d_loop
.d_out:
    pop ebx
    pop esi
    pop edx
    pop ecx
    pop eax
    pop ebp
    ret

; int fdc_wait_cmd(void) — wait until idle and ready for a command byte
; (RQM set, BUSY clear). eax = 1/0.
fdc_wait_cmd:
    push ecx
    push edx
    push esi
    mov esi, 1000000
.loop:
    mov dx, FDC_MSR
    in al, dx
    test al, MSR_BUSY
    jnz .next
    test al, MSR_RQM
    jz .next
    test al, MSR_DIO               ; DIO set => result phase, not command phase
    jnz .next
    mov eax, 1
    jmp .done
.next:
    dec esi
    jnz .loop
    xor eax, eax
.done:
    pop esi
    pop edx
    pop ecx
    ret

; int fdc_wait_idle(void) — wait for the controller to finish executing the
; current command (BSY clear). Seek/recalibrate are driven by the emulated
; stepper motor and take real time, so this polls with a bounded count.
fdc_wait_idle:
    push ecx
    push edx
    push esi
    mov esi, 2000000
.loop:
    mov dx, FDC_MSR
    in al, dx
    test al, MSR_BUSY
    jz .ok
    dec esi
    jnz .loop
    xor eax, eax
    pop esi
    pop edx
    pop ecx
    ret
.ok:
    mov eax, 1
    pop esi
    pop edx
    pop ecx
    ret

; int fdc_wait_ready(void) — settle before touching the FDC again.
; QEMU's DOR never reports drive-ready in bit 7, so polling that bit would spin
; for millions of iterations and never succeed. The motor-on delay in
; floppy_init is what actually covers spin-up.
fdc_wait_ready:
    push dword 20
    call fdc_delay_ms
    add esp, 4
    mov eax, 1
    ret

; int fdc_wait_result(void) — wait until the result phase is ready
; (RQM set, DIO set, BUSY clear). eax = 1/0.
fdc_wait_result:
    push ecx
    push edx
    push esi
    mov esi, 1000000
.loop:
    mov dx, FDC_MSR
    in al, dx
    test al, MSR_BUSY
    jnz .next
    test al, MSR_RQM
    jz .next
    test al, MSR_DIO
    jz .next
    mov eax, 1
    jmp .done
.next:
    dec esi
    jnz .loop
    xor eax, eax
.done:
    pop esi
    pop edx
    pop ecx
    ret

; int fdc_get_result(uint32_t nbytes) — read nbytes result-phase bytes,
; leaving ST0 in fdc_st0. Each read must wait for RQM, otherwise the port read
; races ahead of the controller and desynchronises the phase.
fdc_get_result:
    push ebp
    mov ebp, esp
    push eax
    push ecx
    push edx
    push esi
    mov esi, [ebp + 8]
    xor ecx, ecx
.byte_loop:
    call fdc_wait_result
    mov dx, FDC_DATA
    in al, dx
    test ecx, ecx
    jnz .not_st0
    mov [fdc_st0], al
.not_st0:
    inc ecx
    cmp ecx, esi
    jb .byte_loop
    mov eax, 1
    pop esi
    pop edx
    pop ecx
    pop eax
    pop ebp
    ret

; int fdc_sense_int(void) — fetch the ST0/PCN result of the last SEEK or
; RECALIBRATE. Those commands signal completion with an interrupt and leave the
; MSR idle (DIO clear) rather than presenting a result phase, so waiting for DIO
; alone spins forever. SENSE INTERRUPT STATUS is what actually returns the two
; result bytes, and it also acknowledges the interrupt.
fdc_sense_int:
    push ebp
    mov ebp, esp
    mov eax, CMD_SENSE_INT
    call fdc_send
    push dword 50
    call fdc_delay_ms
    add esp, 4
    call fdc_wait_result
    test eax, eax
    jz .sense_fail
    push dword 2
    call fdc_get_result
    add esp, 4
    mov eax, 1
    jmp .sense_out
.sense_fail:
    xor eax, eax
.sense_out:
    pop ebp
    ret

; ---- DMA channel 2 ------------------------------------------------------

; void fdc_dma_program(uint32_t mode) — program channel 2 to move 512 bytes
; to/from the bounce buffer in the given direction.
fdc_dma_program:
    push ebp
    mov ebp, esp
    push eax
    push ebx

    mov ebx, [ebp + 8]

    ; Mask channel 2 while the address/count/mode registers are in flux.
    mov dx, DMA_MASK
    mov al, DMA_CH2_MASK
    out dx, al

    mov dx, DMA_FF
    xor al, al
    out dx, al                     ; reset the address/count flip-flop

    mov dx, DMA_MODE
    mov al, bl
    out dx, al                     ; transfer mode

    mov dx, DMA2_ADDR
    mov al, FDC_DMA_BUF & 0xFF     ; low byte
    out dx, al
    mov eax, FDC_DMA_BUF
    shr eax, 8
    out dx, al                     ; high byte

    mov dx, DMA2_PAGE
    mov al, (FDC_DMA_BUF >> 16) & 0xFF
    out dx, al                     ; page (high 4 bits of the 20-bit address)

    mov dx, DMA2_CNT
    mov al, 0xFF                   ; count = 512 - 1
    out dx, al
    mov al, 0x01
    out dx, al

    mov dx, DMA_MASK
    mov al, DMA_CH2_UNMASK
    out dx, al                     ; unmask channel 2, arming the transfer

    pop ebx
    pop eax
    pop ebp
    ret

; ---- 512-byte copies --------------------------------------------------

; void fdc_copy_to_bounce(uint32_t src) — bounce <- src
fdc_copy_to_bounce:
    push ebp
    mov ebp, esp
    push esi
    push edi
    push ecx
    mov esi, [ebp + 8]
    mov edi, FDC_DMA_BUF
    mov ecx, 128
.loop:
    mov eax, [esi]
    mov [edi], eax
    add esi, 4
    add edi, 4
    loop .loop
    pop ecx
    pop edi
    pop esi
    pop ebp
    ret

; void fdc_copy_from_bounce(uint32_t dst) — dst <- bounce
fdc_copy_from_bounce:
    push ebp
    mov ebp, esp
    push esi
    push edi
    push ecx
    mov esi, FDC_DMA_BUF
    mov edi, [ebp + 8]
    mov ecx, 128
.loop:
    mov eax, [esi]
    mov [edi], eax
    add esi, 4
    add edi, 4
    loop .loop
    pop ecx
    pop edi
    pop esi
    pop ebp
    ret

; ---- seek --------------------------------------------------------------

; int fdc_seek(void) — seek to io_cyl on head io_head and validate ST0.
fdc_seek:
    push ebp
    mov ebp, esp
    push ebx
    push ecx

    call fdc_wait_cmd
    test eax, eax
    jz .fail_cmd

    mov eax, CMD_SEEK
    call fdc_send
    movzx eax, byte [io_head]
    and eax, 1
    shl eax, 2                      ; HDS bit; drive 0
    call fdc_send
    movzx eax, byte [io_cyl]
    call fdc_send

    call fdc_wait_idle
    test eax, eax
    jz .fail_busy

    call fdc_sense_int
    test eax, eax
    jz .fail_res
    mov al, [fdc_st0]
    test al, 0x03                  ; IC (bits 1:0) must be 00 = normal
    jnz .fail_st0

    mov eax, 1
    jmp .out
.fail_cmd:
    xor eax, eax
    jmp .out
.fail_busy:
    xor eax, eax
    jmp .out
.fail_res:
    xor eax, eax
    jmp .out
.fail_st0:
    xor eax, eax
.out:
    pop ecx
    pop ebx
    pop ebp
    ret

; ---- core CHS access ---------------------------------------------------

; int fdc_transfer(uint32_t lba, uint32_t buf, uint32_t is_write)
; Move one 512-byte sector. eax = 0 on success, -1 on failure.
fdc_transfer:
    push ebp
    mov ebp, esp
    push ebx
    push ecx
    push edx
    push esi
    push edi

    mov esi, [ebp + 8]             ; lba
    mov edi, [ebp + 12]            ; buf

    movzx ecx, byte [floppy_spt]
    movzx edx, byte [floppy_heads]
    test ecx, ecx
    jz .fail
    test edx, edx
    jz .fail

    mov eax, ecx
    imul eax, edx                  ; spt * heads
    mov ebx, eax
    mov eax, esi
    xor edx, edx
    div ebx                        ; eax = cylinder
    mov [io_cyl], al

    mov eax, esi
    xor edx, edx
    movzx ebx, byte [floppy_spt]
    div ebx                        ; eax = lba / spt, edx = lba % spt
    inc edx
    mov [io_sec], dl
    xor edx, edx
    movzx ebx, byte [floppy_heads]
    div ebx                        ; eax = cylinder, edx = head
    mov [io_head], dl

    call fdc_seek
    test eax, eax
    jz .fail_seek
    cmp dword [ebp + 16], 0
    jne .wr

    ; read: DMA will fill the bounce buffer, we copy out after.
    push dword DMA_M_READ
    call fdc_dma_program
    add esp, 4
    jmp .issue
.wr:
    ; write: stage the payload into the bounce buffer first.
    push edi
    call fdc_copy_to_bounce
    add esp, 4
    push dword DMA_M_WRITE
    call fdc_dma_program
    add esp, 4

.issue:
    call fdc_wait_cmd
    test eax, eax
    jz .fail_issue

    cmp dword [ebp + 16], 0
    jne .cmd_write
    mov eax, CMD_READ_DATA
    call fdc_send
    jmp .params
.cmd_write:
    mov eax, CMD_WRITE_DATA
    call fdc_send
.params:
    ; READ/WRITE DATA take eight parameters, in this exact order:
    ;   HD, C, H, R, N, EOT, GPL, DTL
    ; (see the QEMU command table). HD packs the drive in bits 1-0 and the
    ; head in bits 3-2. Sending the bytes in any other order desynchronises
    ; the command: the controller consumes the wrong number of parameters and
    ; never reaches the data phase, leaving the MSR reporting drive-busy.
    movzx eax, byte [io_head]
    shl eax, 2
    or eax, DOR_DRIVE0
    call fdc_send                  ; HD = (head << 2) | drive
    movzx eax, byte [io_cyl]
    call fdc_send                  ; C: cylinder
    movzx eax, byte [io_head]
    call fdc_send                  ; H: head
    movzx eax, byte [io_sec]
    call fdc_send                  ; R: sector
    mov eax, 2
    call fdc_send                  ; N: 512-byte sectors
    movzx eax, byte [floppy_spt]
    call fdc_send                  ; EOT
    mov eax, 0x1B
    call fdc_send                  ; GPL
    mov eax, 0xFF
    call fdc_send                  ; DTL

    call fdc_wait_result
    test eax, eax
    jz .fail_result
    push dword 7
    call fdc_get_result
    add esp, 4
    mov al, [fdc_st0]
    test al, 0x03                  ; IC (bits 1:0) must be 00 = normal
    jnz .fail_st0

    ; --- copy out on read ---
    cmp dword [ebp + 16], 0
    jne .ok
    push edi
    call fdc_copy_from_bounce
    add esp, 4

.ok:
    xor eax, eax
    jmp .out
.fail_seek:
    jmp .fail
.fail_issue:
    jmp .fail
.fail_result:
    jmp .fail
.fail_st0:
.fail:
    mov eax, -1
.out:
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop ebp
    ret

; ---- init --------------------------------------------------------------

; int floppy_init(void) — reset the controller, spin the motor, recalibrate,
; then read the geometry from the volume's BPB. Returns 0 on success.
floppy_init:
    push ebp
    mov ebp, esp
    push ebx
    push ecx
    push edx

    ; LBA 0 maps to C=0,H=0,S=1 regardless of geometry; seed defaults so the
    ; below BPB read divides by a nonzero factor before we know spt/heads.
    mov byte [floppy_spt], 18
    mov byte [floppy_heads], 2

    ; Reset the controller. The reset bit lives in DOR at 0x3F2; pulsing it at
    ; 0x3F6 (the read-only disk-change register) does nothing, which is why the
    ; drive never spun up and every seek hung BUSY.
    mov dx, FDC_DOR
    xor al, al
    out dx, al                      ; reset asserted (bit 2 low)
    push dword 50
    call fdc_delay_ms
    add esp, 4

    mov dx, FDC_DOR
    mov al, DOR_RESET | DOR_DMA_IRQ
    out dx, al                      ; reset released
    call fdc_wait_ready

    ; A reset leaves one interrupt pending per drive: read and discard them.
    mov dword [fdc_int_pending], 4
.int_loop:
    mov eax, CMD_SENSE_INT
    call fdc_send
    push dword 20
    call fdc_delay_ms
    add esp, 4
    call fdc_wait_result
    push dword 2
    call fdc_get_result
    add esp, 4
    dec dword [fdc_int_pending]
    jnz .int_loop

    ; SPECIFY is deliberately skipped: the 82077 powers up with usable stepper
    ; defaults (SRT=2ms, HLT=10ms, HUT=15ms, 2-step unload), and its
    ; four-parameter phase is easy to desynchronise.

    ; Motor on, then give it real time to reach full speed.
    mov dx, FDC_DOR
    mov al, DOR_MOTOR0 | DOR_RESET | DOR_DMA_IRQ
    out dx, al
    push dword 500
    call fdc_delay_ms
    add esp, 4
    call fdc_wait_ready

    ; RECALIBRATE to track 0.
    mov eax, CMD_RECALIB
    call fdc_send
    xor eax, eax                    ; drive 0
    call fdc_send
    push dword 300
    call fdc_delay_ms
    add esp, 4
    call fdc_sense_int

    ; Read sector 0 to pick up the BPB (directly into the bounce buffer).
    push dword 0                    ; lba
    push dword FDC_DMA_BUF          ; buf
    push dword 0                    ; is_write = read
    call fdc_transfer
    add esp, 12
    test eax, eax
    jnz .init_fail

    ; BPB geometry (offset 24 = sectors/track, 26 = heads). A zero in either
    ; field means the read did not land on a boot sector, so refuse to continue
    ; with geometry that would map every LBA onto cylinder 0.
    mov al, [FDC_DMA_BUF + 24]
    test al, al
    jz .init_fail
    mov [floppy_spt], al
    mov al, [FDC_DMA_BUF + 26]
    test al, al
    jz .init_fail
    mov [floppy_heads], al

    mov byte [floppy_ready], 1
    xor eax, eax
    jmp .init_out
.init_fail:
    mov byte [floppy_ready], 0
    mov eax, -1
.init_out:
    pop edx
    pop ecx
    pop ebx
    pop ebp
    ret

; ---- public entry points -----------------------------------------------

; int floppy_read_sector(uint32_t lba, uint32_t buf)
floppy_read_sector:
    push ebp
    mov ebp, esp
    cmp byte [floppy_ready], 0
    jne .go
    call floppy_init
    cmp byte [floppy_ready], 0
    jz .fail
.go:
    push dword 0                    ; is_write = read
    push dword [ebp + 12]
    push dword [ebp + 8]
    call fdc_transfer
    add esp, 12
    pop ebp
    ret
.fail:
    mov eax, -1
    pop ebp
    ret

; int floppy_write_sector(uint32_t lba, uint32_t buf)
floppy_write_sector:
    push ebp
    mov ebp, esp
    cmp byte [floppy_ready], 0
    jne .go
    call floppy_init
    cmp byte [floppy_ready], 0
    jz .fail
.go:
    push dword 1                    ; is_write = write
    push dword [ebp + 12]
    push dword [ebp + 8]
    call fdc_transfer
    add esp, 12
    pop ebp
    ret
.fail:
    mov eax, -1
    pop ebp
    ret

