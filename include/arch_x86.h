#ifndef ARCH_X86_H
#define ARCH_X86_H

#include <stdint.h>

uint8_t  inb(uint16_t port);
void     outb(uint16_t port, uint8_t value);
uint16_t inw(uint16_t port);
void     outw(uint16_t port, uint16_t value);

void disable_interrupts_asm(void);
void enable_interrupts_asm(void);
void task_yield_asm(uint32_t *save_esp, uint32_t load_esp);
void run_scheduler_asm(uint32_t load_esp);
int  disk_read_sector(uint32_t lba, void *buf);
int  disk_write_sector(uint32_t lba, const void *buf);

uint8_t  scancode_to_ascii(uint8_t sc, uint32_t shift);
int      keyboard_poll_scancode(void);
void     system_reboot(void);
void     cpu_pause(void);
uint32_t get_memory_info(void);
uint32_t detect_ps2_memory(void);

#endif
