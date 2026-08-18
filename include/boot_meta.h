#ifndef BOOT_META_H
#define BOOT_META_H

#include <stdint.h>

#define ASMOS_BOOT_MAGIC     "ASMOSBT1"
#define ASMOS_BOOT_VERSION   1
#define ASMOS_KERNEL_MAGIC   0x41534D4Bu  /* 'ASMK' */
#define ASMOS_KERNEL_LOAD    0x00100000u
/* _kernel_start (src/kernel_start.c) is pinned to its own ".text.boot"
 * linker section, listed first inside .text in llinker/linker.ld, which
 * places it at a fixed 0x1000 past the kernel's load address — right
 * after the 4-byte .bootsig magic and the 4K section alignment. This is
 * NOT the same as ASMOS_KERNEL_LOAD: that's where the flat kernel.bin
 * image gets copied to in memory, not where execution should jump to.
 * boot/loader.asm's final far jump uses this same fixed offset directly
 * (as 0x101000) since it has no ELF symbol table to consult at boot
 * time; keep the two in sync if the linker script ever changes. */
#define ASMOS_KERNEL_ENTRY   (ASMOS_KERNEL_LOAD + 0x1000u)
#define ASMOS_STAGE2_LOAD    0x00090000u

typedef struct {
    char     magic[8];
    uint32_t kernel_magic;
    uint32_t kernel_load_addr;
    uint32_t kernel_entry;
    uint32_t kernel_size;
    uint16_t kernel_cluster;
    uint16_t meta_version;
    uint32_t kernel_crc32;
    char     platform[16];
    uint8_t  reserved[460];
} __attribute__((packed)) asmos_boot_meta_t;

#define ASMOS_META_FILENAME  "ASMOS   MET"
#define ASMOS_KERNEL_FILENAME "KERNEL  BIN"
#define ASMOS_CONFIG_FILENAME "CONFIG  WF  "

#endif /* BOOT_META_H */
