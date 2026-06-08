#ifndef BLOCK_DEV_H
#define BLOCK_DEV_H

#include <stdint.h>

#define BLOCK_SECTOR_SIZE 512

typedef struct {
    uint8_t  drive;
    uint16_t sectors_per_track;
    uint16_t heads;
    int      ready;
} block_dev_t;

void block_dev_init(block_dev_t *dev, uint8_t drive);
int  block_dev_read(block_dev_t *dev, uint32_t lba, void *buf);
int  block_dev_write(block_dev_t *dev, uint32_t lba, const void *buf);

#endif /* BLOCK_DEV_H */
