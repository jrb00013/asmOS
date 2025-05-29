#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>

// System calls
void sys_exit(int status);
int sys_read_line(char *buf, int max_len);
void sys_print(const char *str);

// Memory information
uint32_t sys_get_memory_size(void);

#endif // SYSCALLS_H
