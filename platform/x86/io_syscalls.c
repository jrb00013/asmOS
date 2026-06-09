/* x86 syscall layer — bridges shared kernel code to NASM + platform HAL. */

#include "syscalls.h"
#include "platform.h"
#include "kernel.h"
#include <stdint.h>

extern uint32_t get_memory_info(void);
extern void print_string(const char *s);
extern void system_reboot(void);

uint32_t sys_get_memory_size(void) {
    return get_memory_info();
}

void sys_print(const char *str) {
    if (str) print_string(str);
}

void sys_ps2_controller_read(uint8_t *data) {
    plat_controller_state_t st;
    if (!data) return;
    if (plat_controller_read(0, &st) != 0 || !st.present) {
        data[0] = data[1] = data[2] = data[3] = data[4] = data[5] = 0;
        return;
    }
    data[0] = (uint8_t)(st.buttons & 0xFF);
    data[1] = (uint8_t)((st.buttons >> 8) & 0xFF);
    data[2] = (uint8_t)st.lx;
    data[3] = (uint8_t)st.ly;
    data[4] = (uint8_t)st.rx;
    data[5] = (uint8_t)st.ry;
}
