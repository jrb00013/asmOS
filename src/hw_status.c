/* Hardware Status — sysinfo, memstat, ports, iopstat, temp.
 * Kernel & Hardware layer (Johnny). */

#include "hw_status.h"
#include "kernel.h"
#include "memory_manager.h"
#include <stddef.h>

extern uint32_t detect_ps2_memory(void);

#define MEMORY_POOL_SIZE_KB  (1024)  /* 1MB pool from memory_manager.h */

static void memstat_from_system(hw_memstat_t *out) {
    uint32_t total_mb = get_system_memory();
    if (total_mb == 0) {
        uint32_t kb = 0;
        asm volatile("call get_memory_info" : "=a"(kb));
        total_mb = kb / 1024;
    }
    if (total_mb == 0) total_mb = 32;
    out->total_kb = total_mb * 1024;
    out->kernel_kb = 512;   /* approximate */
    out->used_kb = out->kernel_kb + 2048;  /* stub: assume 2MB used */
    if (out->used_kb > out->total_kb) out->used_kb = out->total_kb;
    out->free_kb = out->total_kb - out->used_kb;
}

void hw_status_get_memstat(hw_memstat_t *out) {
    if (!out) return;
    memstat_from_system(out);
}

void hw_status_get_ports(hw_port_status_t *out, unsigned int max) {
    if (!out || max == 0) return;
    for (unsigned int i = 0; i < max && i < HW_STATUS_MAX_PORTS; i++) {
        out[i].present = 1;
        out[i].device_count = (i == 0) ? 1 : 0;  /* port 0: controller */
        const char *p = (i == 0) ? "Controller port 1" : "Controller port 2";
        unsigned int j = 0;
        while (p[j] && j < sizeof(out[i].info) - 1) { out[i].info[j] = p[j]; j++; }
        out[i].info[j] = '\0';
    }
}

void hw_status_get_iop(hw_iop_status_t *out) {
    if (!out) return;
    out->running = 1;
    out->load_percent = 5;  /* stub */
    const char *s = "running";
    unsigned int i = 0;
    while (s[i] && i < sizeof(out->status) - 1) { out->status[i] = s[i]; i++; }
    out->status[i] = '\0';
}

void hw_status_get_network(hw_network_status_t *out) {
    if (!out) return;
    out->linked = 0;  /* stub: no adapter */
    out->ip = 0;
    const char *s = "no link";
    unsigned int i = 0;
    while (s[i] && i < sizeof(out->mac) - 1) { out->mac[i] = s[i]; i++; }
    out->mac[i] = '\0';
    i = 0;
    while (s[i] && i < sizeof(out->info) - 1) { out->info[i] = s[i]; i++; }
    out->info[i] = '\0';
}

void hw_status_get_temp(hw_temp_status_t *out) {
    if (!out) return;
    out->detected = 0;  /* no temp sensor on x86 stub */
    out->celsius = -1;
    const char *s = "n/a";
    unsigned int i = 0;
    while (s[i] && i < sizeof(out->status) - 1) { out->status[i] = s[i]; i++; }
    out->status[i] = '\0';
}

void hw_status_get_sysinfo(hw_sysinfo_t *out) {
    if (!out) return;
    {
        const char *m = "PlayStation 2 (EE)";
        unsigned int i = 0;
        while (m[i] && i < HW_STATUS_MODEL_LEN - 1) { out->model[i] = m[i]; i++; }
        out->model[i] = '\0';
    }
    out->ee_mhz = 294;
    out->ram_mb = 32;
    out->ee_load_percent = 10;  /* stub */
    memstat_from_system(&out->memstat);
    hw_status_get_ports(out->ports, HW_STATUS_MAX_PORTS);
    hw_status_get_iop(&out->iop);
    hw_status_get_network(&out->network);
    hw_status_get_temp(&out->temp);
}
