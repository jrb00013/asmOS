#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stddef.h>

#define PLAT_NAME_MAX       64
#define PLAT_IP_STR_MAX     16
#define PLAT_FILENAME_MAX   12
#define PLAT_SECTOR_SIZE    512

typedef struct {
    char name[PLAT_FILENAME_MAX];
    uint32_t size;
    uint16_t cluster;
} plat_file_info_t;

typedef struct {
    int linked;
    uint32_t ip;
    uint32_t netmask;
    char ip_str[PLAT_IP_STR_MAX];
    char mac_str[24];
} plat_net_info_t;

typedef struct {
    uint16_t buttons;
    int8_t lx, ly, rx, ry;
    int present;
} plat_controller_state_t;

/* Lifecycle */
void plat_init(void);
const char *plat_model_string(void);

/* Console (kernel may also use kprint directly) */
void plat_read_line(char *buf, int max_len);

/* Memory */
uint32_t plat_mem_total_kb(void);
uint32_t plat_mem_used_kb(void);
uint32_t plat_mem_free_kb(void);

/* Timer */
uint32_t plat_ticks_ms(void);
void plat_delay_ms(uint32_t ms);

/* Storage / FAT12 */
int plat_fs_init(void);
int plat_fs_list(plat_file_info_t *out, unsigned int max);
int plat_fs_read(const char *name, void *buf, uint32_t buf_size, uint32_t *out_size);
int plat_fs_write(const char *name, const void *data, uint32_t size);
int plat_fs_delete(const char *name);
int plat_fs_validate(void);
int plat_fs_repair(void);
int plat_fs_read_sector(uint32_t lba, void *buf);
int plat_fs_write_sector(uint32_t lba, const void *buf);

/* Input */
int plat_keyboard_scancode(void);
int plat_keyboard_has_key(void);
int plat_controller_read(int port, plat_controller_state_t *out);

/* Video */
void plat_video_mode_text(void);
void plat_video_mode_13h(void);
void plat_video_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
volatile uint8_t *plat_framebuffer(void);

/* Network */
int plat_net_init(void);
void plat_net_shutdown(void);
int plat_net_send(const void *data, size_t len);
int plat_net_recv(void *buf, size_t max_len);
void plat_net_get_info(plat_net_info_t *out);
int plat_net_ping(const char *host_ip, uint32_t *rtt_ms);

/* System */
void plat_reboot(void);
int plat_temp_celsius(int *out_celsius);

#endif /* PLATFORM_H */
