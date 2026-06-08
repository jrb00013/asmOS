#ifndef FS_H
#define FS_H

#include <stdint.h>

void init_fat12(void);
void fat12_list_files(void);
int fat12_read_file(const char *filename, void *buffer);
int fat12_write_file(const char *filename, const void *buffer, uint32_t size);
int fat12_delete_file(const char *filename);
int fat12_validate_fs(void);

typedef struct {
    char name[11];
    uint32_t size;
    uint16_t cluster;
} file_info_t;

#endif /* FS_H */
