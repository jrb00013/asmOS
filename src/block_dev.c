/* BIOS INT 13h block device for x86 boot and kernel. */

#include "block_dev.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t v;
    __asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

void block_dev_init(block_dev_t *dev, uint8_t drive) {
    if (!dev) return;
    dev->drive = drive;
    dev->sectors_per_track = 18;
    dev->heads = 2;
    dev->ready = 1;
}

static int lba_to_chs(block_dev_t *dev, uint32_t lba, uint8_t *head, uint8_t *sec, uint8_t *cyl) {
    uint32_t spt = dev->sectors_per_track;
    uint32_t hpc = dev->heads;
    uint32_t c, h, s;
    if (!spt || !hpc) return -1;
    s = (lba % spt) + 1;
    c = lba / (spt * hpc);
    h = (lba / spt) % hpc;
    if (c > 255) return -1;
    *head = (uint8_t)h;
    *sec  = (uint8_t)s;
    *cyl  = (uint8_t)c;
    return 0;
}

int block_dev_read(block_dev_t *dev, uint32_t lba, void *buf) {
    uint8_t *p = (uint8_t *)buf;
    if (!dev || !dev->ready || !p) return -1;
    for (int retry = 0; retry < 5; retry++) {
        uint8_t head, sec, cyl;
        if (lba_to_chs(dev, lba, &head, &sec, &cyl) != 0) return -1;
        uint16_t seg = (uint16_t)(((uint32_t)p >> 4) & 0xF000);
        uint16_t off = (uint16_t)((uint32_t)p & 0x0F);
        int err;
        __asm__ volatile (
            "push %%es\n"
            "mov %2, %%ax\n"
            "mov %%ax, %%es\n"
            "mov $0x02, %%ah\n"
            "mov $0x01, %%al\n"
            "mov %3, %%ch\n"
            "mov %4, %%cl\n"
            "mov %5, %%dh\n"
            "mov %6, %%dl\n"
            "mov %7, %%bx\n"
            "int $0x13\n"
            "mov %%es, %%ax\n"
            "pop %%es\n"
            "setc %0\n"
            : "=r"(err), "+b"(off)
            : "r"(seg), "r"(cyl), "r"(sec), "r"(head), "r"(dev->drive), "r"(off)
            : "eax", "ecx", "edx", "memory", "cc"
        );
        if (!err) return 0;
        __asm__ volatile (
            "xor %%ah, %%ah\n"
            "mov %0, %%dl\n"
            "int $0x13\n"
            :
            : "r"(dev->drive)
            : "eax", "edx", "cc"
        );
    }
    return -1;
}

int block_dev_write(block_dev_t *dev, uint32_t lba, const void *buf) {
    const uint8_t *p = (const uint8_t *)buf;
    if (!dev || !dev->ready || !p) return -1;
    for (int retry = 0; retry < 5; retry++) {
        uint8_t head, sec, cyl;
        if (lba_to_chs(dev, lba, &head, &sec, &cyl) != 0) return -1;
        uint16_t seg = (uint16_t)(((uint32_t)p >> 4) & 0xF000);
        uint16_t off = (uint16_t)((uint32_t)p & 0x0F);
        int err;
        __asm__ volatile (
            "push %%es\n"
            "mov %2, %%ax\n"
            "mov %%ax, %%es\n"
            "mov $0x03, %%ah\n"
            "mov $0x01, %%al\n"
            "mov %3, %%ch\n"
            "mov %4, %%cl\n"
            "mov %5, %%dh\n"
            "mov %6, %%dl\n"
            "mov %7, %%bx\n"
            "int $0x13\n"
            "mov %%es, %%ax\n"
            "pop %%es\n"
            "setc %0\n"
            : "=r"(err), "+b"(off)
            : "r"(seg), "r"(cyl), "r"(sec), "r"(head), "r"(dev->drive), "r"(off)
            : "eax", "ecx", "edx", "memory", "cc"
        );
        if (!err) return 0;
        __asm__ volatile (
            "xor %%ah, %%ah\n"
            "mov %0, %%dl\n"
            "int $0x13\n"
            :
            : "r"(dev->drive)
            : "eax", "edx", "cc"
        );
    }
    return -1;
}
