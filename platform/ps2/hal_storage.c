#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <kernel.h>

#define MC_BASE "mc0:/asmos"

static int fs_ready;

static void mc_path(char *dst, int max, const char *name) {
    snprintf(dst, max, "%s/%s", MC_BASE, name);
}

int plat_fs_init(void) {
    mkdir(MC_BASE, 0777);
    fs_ready = 1;
    return 0;
}

int plat_fs_list(plat_file_info_t *out, unsigned int max) {
    if (!fs_ready) plat_fs_init();
    (void)out;
    (void)max;
    /* Full MC directory walk needs fileXio; return empty for now. */
    return 0;
}

int plat_fs_read(const char *name, void *buf, uint32_t buf_size, uint32_t *out_size) {
    char path[128];
    int fd, n;
    if (!name || !buf) return -1;
    mc_path(path, sizeof(path), name);
    fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    n = read(fd, buf, buf_size);
    close(fd);
    if (n < 0) return -1;
    if (out_size) *out_size = (uint32_t)n;
    return 0;
}

int plat_fs_write(const char *name, const void *data, uint32_t size) {
    char path[128];
    int fd, n;
    if (!name || !data) return -1;
    mc_path(path, sizeof(path), name);
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) return -1;
    n = write(fd, data, size);
    close(fd);
    return (n == (int)size) ? 0 : -1;
}

int plat_fs_delete(const char *name) {
    char path[128];
    if (!name) return -1;
    mc_path(path, sizeof(path), name);
    return remove(path);
}

int plat_fs_validate(void) {
    return fs_ready ? 0 : -1;
}

int plat_fs_repair(void) {
    return plat_fs_init();
}

int plat_fs_read_sector(uint32_t lba, void *buf) {
    (void)lba;
    (void)buf;
    return -1;
}

int plat_fs_write_sector(uint32_t lba, const void *buf) {
    (void)lba;
    (void)buf;
    return -1;
}
