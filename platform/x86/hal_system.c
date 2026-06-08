/* x86 platform HAL — system, memory, timer, reboot. */

#include "platform.h"
#include "memory_manager.h"
#include "kernel.h"
#include <stdint.h>

extern uint32_t detect_ps2_memory(void);

static uint32_t tick_ms;

void plat_init(void) {
    tick_ms = 0;
}

const char *plat_model_string(void) {
    return "ASMOS x86 (QEMU/Modchip CD)";
}

void plat_read_line(char *buf, int max_len) {
    extern int sys_read_line(char *buf, int max_len);
    sys_read_line(buf, max_len);
}

uint32_t plat_mem_total_kb(void) {
    uint32_t mb = detect_ps2_memory();
    if (mb == 0) mb = 32;
    return mb * 1024;
}

uint32_t plat_mem_used_kb(void) {
    unsigned int pct = get_memory_usage_percent();
    return (plat_mem_total_kb() * pct) / 100;
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
    for (uint32_t i = 0; i < ms * 1000; i++)
        asm volatile("pause");
    tick_ms += ms;
}

void plat_reboot(void) {
    halt_system();
}

int plat_temp_celsius(int *out_celsius) {
    if (out_celsius) *out_celsius = 45;
    return 0;
}
