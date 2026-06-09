#ifndef ARCH_X86_H
#define ARCH_X86_H

#include <stdint.h>

uint8_t  inb(uint16_t port);
void     outb(uint16_t port, uint8_t value);
uint16_t inw(uint16_t port);
void     outw(uint16_t port, uint16_t value);

void task_yield_asm(uint32_t *save_esp, uint32_t load_esp);
void run_scheduler_asm(uint32_t load_esp);

#endif
