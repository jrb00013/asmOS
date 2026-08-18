#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <kernel.h>

#define MC_BASE "mc0:/asmos"
#define MC_CWD_MAX 128

static int fs_ready;

/* Current directory, relative to MC_BASE ("" == MC_BASE itself). Tracked
 * the same way the x86 HAL tracks cwd_path, so shell.c's cd/ls/pwd
 * commands behave identically on both platforms (HAL parity). PS2SDK's
 * mc0: driver exposes standard POSIX mkdir/opendir/readdir/stat for
 * memory-card paths, so this uses the same calls a normal PS2SDK
 * homebrew app would use for MC file management. */
static char mc_cwd[MC_CWD_MAX] = "";

static void mc_path(char *dst, int max, const char *name) {
    if (mc_cwd[0])
        snprintf(dst, max, "%s/%s/%s", MC_BASE, mc_cwd, name);
    else
        snprintf(dst, max, "%s/%s", MC_BASE, name);
}

static void mc_dir_path(char *dst, int max) {
    if (mc_cwd[0])
        snprintf(dst, max, "%s/%s", MC_BASE, mc_cwd);
    else
        snprintf(dst, max, "%s", MC_BASE);
}

int plat_fs_init(void) {
    mkdir(MC_BASE, 0777);
    fs_ready = 1;
    return 0;
}

int plat_fs_list(plat_file_info_t *out, unsigned int max) {
    if (!fs_ready) plat_fs_init();
    if (!out) return -1;

    char dirpath[MC_CWD_MAX + 16];
    mc_dir_path(dirpath, sizeof(dirpath));

    DIR *d = opendir(dirpath);
    if (!d) return -1;

    unsigned int n = 0;
    struct dirent *ent;
    while (n < max && (ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.' &&
            (ent->d_name[1] == '\0' || (ent->d_name[1] == '.' && ent->d_name[2] == '\0')))
            continue; /* skip "." and ".." like the x86 listing does */

        char full[MC_CWD_MAX + 64];
        snprintf(full, sizeof(full), "%s/%s", dirpath, ent->d_name);
        struct stat st;
        int is_dir = 0;
        uint32_t size = 0;
        if (stat(full, &st) == 0) {
            is_dir = S_ISDIR(st.st_mode);
            size = (uint32_t)st.st_size;
        }

        int i;
        for (i = 0; i < PLAT_FILENAME_MAX - 2 && ent->d_name[i]; i++)
            out[n].name[i] = ent->d_name[i];
        if (is_dir && i < PLAT_FILENAME_MAX - 1) out[n].name[i++] = '/';
        out[n].name[i] = '\0';
        out[n].size = size;
        out[n].cluster = 0;
        n++;
    }
    closedir(d);
    return (int)n;
}

int plat_fs_cwd(char *buf, unsigned int max) {
    if (!buf || max == 0) return -1;
    if (mc_cwd[0] == '\0') {
        if (max < 2) return -1;
        buf[0] = '/'; buf[1] = '\0';
        return 0;
    }
    unsigned int i;
    buf[0] = '/';
    for (i = 0; i + 2 < max && mc_cwd[i]; i++) buf[i + 1] = mc_cwd[i];
    buf[i + 1] = '\0';
    return 0;
}

int plat_fs_mkdir(const char *name) {
    if (!name || !name[0]) return -1;
    if (!fs_ready) plat_fs_init();
    char path[MC_CWD_MAX + 32];
    mc_path(path, sizeof(path), name);
    return mkdir(path, 0777);
}

int plat_fs_chdir(const char *name) {
    if (!fs_ready) plat_fs_init();
    if (!name || !name[0] || (name[0] == '.' && name[1] == '\0')) return 0;

    if (name[0] == '/' && name[1] == '\0') {
        mc_cwd[0] = '\0';
        return 0;
    }

    if (name[0] == '.' && name[1] == '.' && name[2] == '\0') {
        if (!mc_cwd[0]) return 0; /* already at MC_BASE */
        int len = (int)strlen(mc_cwd);
        while (len > 0 && mc_cwd[len - 1] != '/') len--;
        if (len > 0) len--; /* drop trailing '/' */
        mc_cwd[len < 0 ? 0 : len] = '\0';
        return 0;
    }

    char candidate[MC_CWD_MAX];
    if (mc_cwd[0])
        snprintf(candidate, sizeof(candidate), "%s/%s", mc_cwd, name);
    else
        snprintf(candidate, sizeof(candidate), "%s", name);

    char full[MC_CWD_MAX + 64];
    snprintf(full, sizeof(full), "%s/%s", MC_BASE, candidate);
    struct stat st;
    if (stat(full, &st) != 0 || !S_ISDIR(st.st_mode)) return -1;

    strncpy(mc_cwd, candidate, sizeof(mc_cwd) - 1);
    mc_cwd[sizeof(mc_cwd) - 1] = '\0';
    return 0;
}

int plat_fs_read(const char *name, void *buf, uint32_t buf_size, uint32_t *out_size) {
    char path[256];
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
    char path[256];
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
    char path[256];
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
