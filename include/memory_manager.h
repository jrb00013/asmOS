#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stddef.h>  // For size_t

#define MEMORY_POOL_SIZE 1024  // Adjust as needed

// Memory block structure for simple memory management
typedef struct mem_block {
    size_t size;
    struct mem_block *next;
    int free;
} mem_block_t;

// Function prototypes
void init_memory_manager(void);
void *malloc(size_t size);
void free(void *ptr);

#endif // MEMORY_MANAGER_H
