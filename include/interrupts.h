#ifndef ASMOS_INTERRUPTS_H
#define ASMOS_INTERRUPTS_H

#include <stdint.h>

/* Register frame saved by the common stub in boot/arch_x86/idt.asm.
 * Layout matches the push order in isr_common, so the C dispatcher can
 * read these fields directly off the stack pointer it is handed. */
typedef struct {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp_dummy;
    uint32_t ebx, edx, ecx, eax;
    uint32_t err_code;
    uint32_t vector;
    uint32_t eip, cs, eflags;
} isr_regs_t;

#ifdef __cplusplus
extern "C" {
#endif

void interrupt_init(void);
void pic_unmask(uint8_t irq);

extern uint32_t interrupt_counters[16];    /* per-IRQ count, index = vector - 32 */
extern uint32_t exception_counters[32];   /* per-exception count, index = vector */
extern uint32_t saved_pic_masks;

#ifdef __cplusplus
}
#endif

#endif /* ASMOS_INTERRUPTS_H */