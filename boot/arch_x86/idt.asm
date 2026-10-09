; Minimal but real 256-entry IDT + common interrupt entry.
;
; kernel_main() calls enable_interrupts_asm() (sti) before entering the
; shell, but nothing in the whole project ever executed LIDT. SeaBIOS leaves
; IDTR pointing at its own real-mode IVT copy, whose entries are 16-bit
; far pointers, not 32-bit gate descriptors. The moment IF was set, the
; first interrupt or exception vectored through that bogus IDT into
; garbage and the guest took a double fault on top of it — a triple
; fault. That is why the boot test's "reached kernel entry" marker was the
; last observable thing: QEMU reset immediately after KERNEL_START.
;
; This file installs a real IDT so interrupts are survivable, and routes
; every vector to one common entry that hands the vector plus the pushed
; register frame to isr_dispatch() in C.
[BITS 32]

section .note.GNU-stack noalloc noexec nowrite progbits
section .text

global idt_init

extern isr_dispatch

%define KERNEL_CODE_SEL 0x08
%define KERNEL_DATA_SEL 0x0A
%define GATE_32_INT     0x8E

; Vectors the CPU pushes an error code for. Every other vector enters
; with none, so a dummy 0 is pushed to keep the frame layout uniform.
%macro ISR_NOERR 1
global isr_stub_%1
isr_stub_%1:
    push dword 0
    push dword %1
    jmp isr_common
%endmacro

%macro ISR_ERR 1
global isr_stub_%1
isr_stub_%1:
    push dword %1
    jmp isr_common
%endmacro

%macro STUB_ADDR 1
    dd isr_stub_%1
%endmacro

; --- 256 stubs -----------------------------------------------------------
%assign vec 0
%rep 256
    %if (vec == 8) || (vec == 10) || (vec == 11) || (vec == 12) || \
        (vec == 13) || (vec == 14) || (vec == 17) || (vec == 21) || \
        (vec == 29) || (vec == 30)
        ISR_ERR vec
    %else
        ISR_NOERR vec
    %endif
%assign vec vec + 1
%endrep

; --- single common entry -------------------------------------------------
; Stack on entry, top first:
;   vector, error_code, edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax
; then the CPU-pushed eip, cs, eflags.
isr_common:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, KERNEL_DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; argv[1] = vector, argv[0] = pointer to the saved register frame.
    ; [esp] holds saved gs (pushed after pusha), so the register block
    ; begins at [esp + 4]. The vector sits 13 dwords further up:
    ; 4 segment saves + 8 pusha regs + 1 error code.
    mov eax, esp
    mov eax, [eax + 52]
    push eax
    lea eax, [esp + 4]
    push eax
    call isr_dispatch
    add esp, 8

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8                           ; discard vector + error code
    iret

; --- the table itself ----------------------------------------------------
; NASM can't shift a label's value at assembly time to split it into the
; IDT gate's low/high offset halves, so the stub addresses live here as
; plain 32-bit values and idt_init() splices them into gate descriptors
; at runtime. 256 dwords = 1KB of table, built once at boot.
section .rodata
align 4
stub_table:
%assign vec 0
%rep 256
    STUB_ADDR vec
%assign vec vec + 1
%endrep
stub_table_end:

section .bss
align 8
idt:
    resb 256 * 8
idt_end:

section .text
align 4
idt_descriptor:
    dw idt_end - idt - 1
    dd idt

; void idt_init(void) — build all 256 gates (32-bit interrupt gate, DPL 0,
; selector 0x08) and load IDTR. Interrupts stay masked; kernel_main
; enables them once the PIC is remapped out of the CPU exception range.
idt_init:
    cli
    push esi
    push edi
    mov edi, idt
    mov esi, stub_table
    mov ecx, 256
.gate_loop:
    mov eax, [esi]
    mov word [edi], ax                   ; offset low 16
    mov word [edi + 2], KERNEL_CODE_SEL
    mov byte [edi + 4], 0                ; IST 0
    mov byte [edi + 5], GATE_32_INT
    shr eax, 16
    mov word [edi + 6], ax               ; offset high 16
    add edi, 8
    add esi, 4
    loop .gate_loop
    pop edi
    pop esi
    lidt [idt_descriptor]
    ret