/* PS2 replacements for x86 ASM symbols used by shared kernel code. */

#ifdef PLATFORM_PS2

#include "platform.h"
#include "../../include/kernel.h"
#include "fs.h"
#include <stdint.h>

void disable_interrupts_asm(void) { }
void enable_interrupts_asm(void) { }
void load_kernel_from_disk(void) { }

uint32_t detect_ps2_memory(void) {
    return 32;
}

void init_ps2_controllers(void) {
    plat_controller_state_t st;
    plat_controller_read(0, &st);
}

void init_fat12(void) {
    plat_fs_init();
}

void fat12_list_files(void) {
    plat_file_info_t files[32];
    int n = plat_fs_list(files, 32);
    int i;
    if (n < 0) {
        kprint("  fs: list failed\n");
        return;
    }
    for (i = 0; i < n; i++) {
        kprintf("  %s  %u bytes\n", files[i].name, (unsigned)files[i].size);
    }
    kprintf("  %d file(s)\n", n);
}

int fat12_read_file(const char *filename, void *buffer) {
    uint32_t sz;
    if (plat_fs_read(filename, buffer, 65536, &sz) != 0) return -1;
    return (int)sz;
}

int fat12_write_file(const char *filename, const void *buffer, uint32_t size) {
    return plat_fs_write(filename, buffer, size);
}

int fat12_delete_file(const char *filename) {
    return plat_fs_delete(filename);
}

int fat12_validate_fs(void) {
    return plat_fs_validate();
}

#endif /* PLATFORM_PS2 */
