/* x86 interrupt dispatch: PIC remap, EOI, and exception reporting.
 *
 * boot/arch_x86/idt.asm installs a 256-gate IDT and funnels every vector
 * here. Hardware IRQs must not land on vectors 0x08-0x0F, which the 8259
 * PICs default to — those are CPU exception vectors (#DF, #GP, ...), so an
 * unmasked timer IRQ would be interpreted as a fault. pic_init() moves
 * the master PIC to 0x20 and the slave to 0x28, the standard remap.
 *
 * IRQ0 (PIT) is deliberately left masked. Nothing in the kernel consumes
 * timer interrupts — sys_timer_get() reads the TSC directly
 * (boot/arch_x86/timer.asm) and keyboard input is polled synchronously
 * out of port 0x60 — so unmasking it would only add interrupt traffic
 * that no handler has work to do.
 */
#ifndef PLATFORM_PS2

#include <stdint.h>
#include "arch_x86.h"
#include "kernel.h"
#include "interrupts.h"

#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1

#define PIC1_VECTOR_BASE 0x20
#define PIC2_VECTOR_BASE 0x28

#define PIC_EOI     0x20
#define PIC_READ_ISR 0x0B

extern void idt_init(void);
extern void isr_dispatch(uint32_t vector, isr_regs_t *regs);

uint32_t interrupt_counters[16];
uint32_t exception_counters[32];
uint32_t saved_pic_masks;

void pic_init(void) {
    /* Record the masks the BIOS left behind, then mask everything while
     * the remap is in flight — an IRQ arriving mid-sequence would vector
     * through a half-built PIC state. */
    saved_pic_masks = ((uint32_t)inb(PIC2_DATA) << 8) | inb(PIC1_DATA);

    outb(PIC1_CMD, 0x11);             /* ICW1: init + expect ICW4 */
    outb(PIC1_DATA, PIC1_VECTOR_BASE);
    outb(PIC1_DATA, PIC2_VECTOR_BASE);
    outb(PIC1_DATA, 0x04);            /* slave is wired to master IRQ2 */
    outb(PIC1_DATA, 0x02);            /* 8086 mode, normal cascade order */

    outb(PIC2_CMD, 0x11);
    outb(PIC2_DATA, PIC2_VECTOR_BASE);
    outb(PIC2_DATA, PIC2_VECTOR_BASE);
    outb(PIC2_DATA, 0x02);            /* slave cascade identity */

    outb(PIC1_DATA, 0xFF);            /* mask all until explicitly unmasked */
    outb(PIC2_DATA, 0xFF);
}

void pic_unmask(uint8_t irq) {
    if (irq >= 8)
        outb(PIC2_DATA, (uint8_t)(inb(PIC2_DATA) & ~(1u << (irq - 8))));
    else
        outb(PIC1_DATA, (uint8_t)(inb(PIC1_DATA) & ~(1u << irq)));
}

static void pic_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_CMD, PIC_EOI);
        outb(PIC1_CMD, PIC_EOI);
    } else {
        outb(PIC1_CMD, PIC_EOI);
    }
}

static int pic_spurious(uint8_t irq, uint8_t cmd_port) {
    if (irq != 7 && irq != 15) return 0;
    outb(cmd_port, PIC_READ_ISR);
    if (inb(cmd_port) & (1u << (irq & 7))) return 0;   /* real IRQ in flight */
    return 1;                                        /* spurious: no EOI owed */
}

static void append_hex32(char *buf, uint32_t *i, uint32_t max, uint32_t v) {
    static const char hex[] = "0123456789ABCDEF";
    for (int shift = 28; shift >= 0; shift -= 4) {
        if (*i + 1 >= max) return;
        buf[(*i)++] = hex[(v >> shift) & 0xF];
    }
}

/* Print a CPU exception's vector, faulting EIP, and error code. Without
 * this the shell would silently swallow a #PF or #UD and keep running on
 * whatever state that left behind. */
static void exception_report(uint32_t vector, const isr_regs_t *r) {
    char buf[96];
    uint32_t i = 0;

    buf[i++] = 'E'; buf[i++] = 'X'; buf[i++] = 'C';
    buf[i++] = (char)('0' + (vector / 10) % 10);
    buf[i++] = (char)('0' + vector % 10);
    buf[i++] = ' ';
    append_hex32(buf, &i, sizeof(buf), r->eip);
    buf[i++] = ' ';
    append_hex32(buf, &i, sizeof(buf), r->err_code);
    buf[i] = '\0';

    kprint("\n  ");
    kprint_color("exception ", 0x0C);
    kprint_color(buf, 0x0E);
    kprint("\n");
}

void interrupt_init(void) {
    idt_init();
    pic_init();
}

void isr_dispatch(uint32_t vector, isr_regs_t *r) {
    if (vector >= 32 && vector < 48) {
        uint8_t irq = (uint8_t)(vector - 32);
        if (!pic_spurious(irq, irq >= 8 ? PIC2_CMD : PIC1_CMD))
            pic_eoi(irq);
        interrupt_counters[irq]++;
        return;
    }
    if (vector < 32) {
        exception_counters[vector]++;
        exception_report(vector, r);
    }
    /* Vectors 0x30-0x7F are BIOS/IRQ aliases with no handler here. Count
     * and drop them rather than treating them as faults. */
}

#else

/* Keep the translation unit non-empty for the PS2 build. */
int asmos_interrupts_ps2_unused(void);

#endif /* !PLATFORM_PS2 */