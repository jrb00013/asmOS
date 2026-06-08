#ifndef BOOT_META_H
#define BOOT_META_H

#include <stdint.h>

#define ASMOS_BOOT_MAGIC     "ASMOSBT1"
#define ASMOS_BOOT_VERSION   1
#define ASMOS_KERNEL_MAGIC   0x41534D4Bu  /* 'ASMK' */
#define ASMOS_KERNEL_LOAD    0x00100000u
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
