#include "platform.h"
#include "memory_manager.h"
#include <kernel.h>
#include <delaythread.h>
#include <debug.h>
#include <timer.h>
#include <stdint.h>

static uint32_t tick_ms;

void plat_init(void) {
    tick_ms = 0;
}

const char *plat_model_string(void) {
    return "ASMOS PS2 Native (EE/FMCB)";
}

void plat_read_line(char *buf, int max_len) {
    extern int sys_read_line(char *buf, int max_len);
    sys_read_line(buf, max_len);
}

uint32_t plat_mem_total_kb(void) {
    return 32 * 1024;
}

uint32_t plat_mem_used_kb(void) {
    return (plat_mem_total_kb() * get_memory_usage_percent()) / 100;
}

uint32_t plat_mem_free_kb(void) {
    uint32_t t = plat_mem_total_kb();
    uint32_t u = plat_mem_used_kb();
    return (u < t) ? (t - u) : 0;
}

uint32_t plat_ticks_ms(void) {
    return tick_ms;
}

void plat_delay_ms(uint32_t ms) {
    DelayThread(ms * 1000);
    tick_ms += ms;
}

void plat_reboot(void) {
    scr_printf("Reboot not implemented on PS2 — reset console.\n");
}

int plat_temp_celsius(int *out_celsius) {
    if (out_celsius) *out_celsius = 55;
    return 0;
}
