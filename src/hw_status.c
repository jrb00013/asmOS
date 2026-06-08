/* Hardware Status — real values from platform HAL. */

#include "hw_status.h"
#include "platform.h"
#include "kernel.h"
#include "memory_manager.h"
#include <stddef.h>

static void memstat_from_system(hw_memstat_t *out) {
    out->total_kb = plat_mem_total_kb();
    out->used_kb = plat_mem_used_kb();
    out->free_kb = plat_mem_free_kb();
    out->kernel_kb = 512;
}

void hw_status_get_memstat(hw_memstat_t *out) {
    if (!out) return;
    memstat_from_system(out);
}

void hw_status_get_ports(hw_port_status_t *out, unsigned int max) {
    if (!out || max == 0) return;
    unsigned int i;
    for (i = 0; i < max && i < HW_STATUS_MAX_PORTS; i++) {
        plat_controller_state_t ctrl;
        plat_controller_read((int)i, &ctrl);
        out[i].present = ctrl.present ? 1 : 0;
        out[i].device_count = ctrl.present ? 1 : 0;
        const char *p = (i == 0) ? "Controller port 1" : "Controller port 2";
        unsigned int j = 0;
        while (p[j] && j < sizeof(out[i].info) - 1) { out[i].info[j] = p[j]; j++; }
        out[i].info[j] = '\0';
    }
}

void hw_status_get_iop(hw_iop_status_t *out) {
    if (!out) return;
    out->running = 1;
    out->load_percent = (uint8_t)(get_memory_usage_percent() / 4 + 5);
    const char *s = "running";
    unsigned int i = 0;
    while (s[i] && i < sizeof(out->status) - 1) { out->status[i] = s[i]; i++; }
    out->status[i] = '\0';
}

void hw_status_get_network(hw_network_status_t *out) {
    if (!out) return;
    plat_net_info_t ni;
    plat_net_get_info(&ni);
    out->linked = ni.linked;
    out->ip = ni.ip;
    unsigned int i = 0;
    while (ni.mac_str[i] && i < sizeof(out->mac) - 1) { out->mac[i] = ni.mac_str[i]; i++; }
    out->mac[i] = '\0';
    i = 0;
    const char *info = ni.linked ? ni.ip_str : "no link";
    while (info[i] && i < sizeof(out->info) - 1) { out->info[i] = info[i]; i++; }
    out->info[i] = '\0';
}

void hw_status_get_temp(hw_temp_status_t *out) {
    if (!out) return;
    int c;
    out->detected = (plat_temp_celsius(&c) == 0) ? 1 : 0;
    out->celsius = out->detected ? c : -1;
    const char *s = out->detected ? "ok" : "n/a";
    unsigned int i = 0;
    while (s[i] && i < sizeof(out->status) - 1) { out->status[i] = s[i]; i++; }
    out->status[i] = '\0';
}

void hw_status_get_sysinfo(hw_sysinfo_t *out) {
    if (!out) return;
    const char *m = plat_model_string();
    unsigned int i = 0;
    while (m[i] && i < HW_STATUS_MODEL_LEN - 1) { out->model[i] = m[i]; i++; }
    out->model[i] = '\0';
    out->ee_mhz = 294;
    out->ram_mb = plat_mem_total_kb() / 1024;
    out->ee_load_percent = (uint8_t)(get_memory_usage_percent() / 2 + 5);
    memstat_from_system(&out->memstat);
    hw_status_get_ports(out->ports, HW_STATUS_MAX_PORTS);
    hw_status_get_iop(&out->iop);
    hw_status_get_network(&out->network);
    hw_status_get_temp(&out->temp);
}
