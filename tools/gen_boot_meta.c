/* Generate ASMOS.META and patch kernel CRC for boot verification. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../include/boot_meta.h"

static uint32_t crc32(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc >> 1) ^ (0xEDB88320u & (-(crc & 1)));
    }
    return ~crc;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "usage: %s <kernel.bin> <out.meta> <platform>\n", argv[0]);
        return 1;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc((size_t)sz);
    if (!buf) return 1;
    fread(buf, 1, (size_t)sz, f);
    fclose(f);

    asmos_boot_meta_t meta;
    memset(&meta, 0, sizeof(meta));
    memcpy(meta.magic, ASMOS_BOOT_MAGIC, 8);
    meta.kernel_magic = ASMOS_KERNEL_MAGIC;
    meta.kernel_load_addr = ASMOS_KERNEL_LOAD;
    meta.kernel_entry = ASMOS_KERNEL_LOAD;
    meta.kernel_size = (uint32_t)sz;
    meta.kernel_cluster = 2;
    meta.meta_version = ASMOS_BOOT_VERSION;
    meta.kernel_crc32 = crc32(buf, (size_t)sz);
    strncpy(meta.platform, argv[3], sizeof(meta.platform) - 1);

    FILE *o = fopen(argv[2], "wb");
    if (!o) return 1;
    fwrite(&meta, 1, sizeof(meta), o);
    fclose(o);
    free(buf);
    printf("meta: size=%ld crc=0x%08x\n", sz, meta.kernel_crc32);
    return 0;
}
