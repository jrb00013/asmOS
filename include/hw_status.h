#ifndef HW_STATUS_H
#define HW_STATUS_H

#include <stdint.h>

/* Hardware Status CLI — Kernel & Hardware layer (Johnny).
 * EE CPU load, RAM, IOP, USB, network, memory port status.
 * Works on current x86/BIOS stack; real PS2 fills via HAL. */

#define HW_STATUS_MAX_PORTS   2
#define HW_STATUS_MODEL_LEN  32

typedef struct {
    uint32_t total_kb;
    uint32_t free_kb;
    uint32_t used_kb;
    uint32_t kernel_kb;
} hw_memstat_t;

typedef struct {
    int present;
    int device_count;
    char info[64];
} hw_port_status_t;

typedef struct {
    int running;
    uint32_t load_percent;   /* 0–100 */
    char status[32];
} hw_iop_status_t;

typedef struct {
    int linked;
    uint32_t ip;             /* IPv4 host order */
    char mac[18];
    char info[64];
} hw_network_status_t;

typedef struct {
    int detected;
    int celsius;             /* -1 if N/A */
    char status[16];         /* "ok" / "hot" / "critical" */
} hw_temp_status_t;

typedef struct {
    char model[HW_STATUS_MODEL_LEN];
    uint32_t ee_mhz;
    uint32_t ram_mb;
    uint32_t ee_load_percent;    /* 0–100, estimated */
    hw_memstat_t memstat;
    hw_port_status_t ports[HW_STATUS_MAX_PORTS];
    hw_iop_status_t iop;
    hw_network_status_t network;
    hw_temp_status_t temp;
} hw_sysinfo_t;

/* Populate full system info (for sysinfo CLI). */
void hw_status_get_sysinfo(hw_sysinfo_t *out);

/* Individual queries (for memstat, ports, iopstat, temp). */
void hw_status_get_memstat(hw_memstat_t *out);
void hw_status_get_ports(hw_port_status_t *out, unsigned int max);
void hw_status_get_iop(hw_iop_status_t *out);
void hw_status_get_temp(hw_temp_status_t *out);
void hw_status_get_network(hw_network_status_t *out);

#endif /* HW_STATUS_H */
