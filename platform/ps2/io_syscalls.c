/* PS2 syscalls: line input and controller read for shared shell/game code. */

#ifdef PLATFORM_PS2

#include "syscalls.h"
#include "platform.h"
#include <kernel.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>

int sys_read_line(char *buf, int max_len) {
    if (!buf || max_len <= 0) return 0;
    int i = 0;
    while (i < max_len - 1) {
        int c = getchar();
        if (c == EOF) {
            plat_delay_ms(10);
            continue;
        }
        if (c == '\r' || c == '\n') {
            buf[i] = '\0';
            scr_printf("\n");
            return i;
        }
        if (c == 8 || c == 127) {
            if (i > 0) { i--; scr_printf("\b \b"); }
            continue;
        }
        buf[i++] = (char)c;
        scr_printf("%c", c);
    }
    buf[i] = '\0';
    return i;
}

uint32_t sys_get_memory_size(void) {
    return plat_mem_total_kb();
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

void sys_sound_init(void) { }
void sys_graphics_init(void) { }
void sys_timer_init(void) { }
uint32_t sys_timer_get(void) { return plat_ticks_ms(); }

void sys_exit(int status) {
    (void)status;
    scr_printf("ASMOS exit.\n");
    SleepThread();
}

#endif /* PLATFORM_PS2 */
