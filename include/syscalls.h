#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>

void sys_exit(int status);
int sys_read_line(char *buf, int max_len);
void sys_print(const char *str);
uint32_t sys_get_memory_size(void);

#ifdef PLATFORM_PS2
void sys_sound_init(void);
void sys_graphics_init(void);
void sys_timer_init(void);
uint32_t sys_timer_get(void);
void sys_ps2_controller_read(uint8_t *data);
#endif

#endif /* SYSCALLS_H */
