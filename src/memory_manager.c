#include "memory_manager.h"

static char memory_pool[MEMORY_POOL_SIZE];
static int memory_index = 0;

// Function to initialize memory pool
void init_memory_manager(void) {
    memory_index = 0;  // Start from the beginning of the memory pool
}

// Function to allocate memory dynamically
void *malloc(size_t size) {
    if (memory_index + size > MEMORY_POOL_SIZE) {
        return NULL;  // Out of memory
    }
    void *allocated_memory = &memory_pool[memory_index];
    memory_index += size;  // Increment memory index
    return allocated_memory;
}

// Function to free allocated memory (in a simple OS, this is often omitted)
void free(void *ptr) {
    // Simple implementation: In real OS, we'd mark memory as free
    // This version doesn't support freeing memory.
}
