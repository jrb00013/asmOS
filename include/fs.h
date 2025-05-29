#ifndef FS_H
#define FS_H

#include <stdint.h>

// FAT12 functions
void init_fat12(void);
void fat12_list_files(void);
int fat12_read_file(const char *filename, void *buffer);

// File operations
typedef struct {
    char name[11];      // 8.3 format
    uint32_t size;
    uint16_t cluster;
} file_info_t;

#endif // FS_H
