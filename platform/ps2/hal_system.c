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

/* LoadExecPS2() is the real PS2SDK EE-kernel call (declared in the PS2SDK
 * <kernel.h> pulled in above, syscall __NR__LoadExecPS2) used to hand
 * control to another ELF — it is the same mechanism homebrew loaders use
 * for a "return to browser"/soft-reset path. Booting "rom0:PS2LOGO" hands
 * control back to the console's own boot ROM logo app, which is the
 * standard PS2SDK idiom for a software reboot when there is no dedicated
 * hardware reset line to pull (unlike a real power-cycle). It is declared
 * __attribute__((noreturn)) and does not return on real hardware or under
 * FreeMCBoot; the loop below is only a safety net if a given BIOS/loader
 * combination fails to take over.
 *
 * src/kernel.c's halt_system() calls system_reboot() directly (mirroring
 * the x86 arch, where system_reboot is the NASM real-mode reset routine),
 * so that entry point is implemented here too — plat_reboot() just calls
 * it, exactly like platform/x86/hal_system.c's plat_reboot() calls the
 * NASM system_reboot(). */
void system_reboot(void) {
    scr_printf("Rebooting PS2...\n");
    DelayThread(500000); /* let the message reach the screen before reset */
    LoadExecPS2("rom0:PS2LOGO", 0, NULL);
    while (1) {
        DelayThread(1000000);
    }
}

void plat_reboot(void) {
    system_reboot();
}

int plat_temp_celsius(int *out_celsius) {
    if (out_celsius) *out_celsius = 55;
    return 0;
}
