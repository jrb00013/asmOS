#include "memory_manager.h"
#include "kernel.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static char memory_pool[MEMORY_POOL_SIZE];
static mem_block_t *free_list = NULL;


#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))


void init_memory_manager(void) {
    free_list = (mem_block_t *)memory_pool;
    free_list->size = MEMORY_POOL_SIZE - sizeof(mem_block_t);
    free_list->free = 1;
    free_list->next = NULL;
}


static mem_block_t *find_free_block(size_t size) {
    mem_block_t *current = free_list;
    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}


static void split_block(mem_block_t *block, size_t size) {
    if (block->size >= size + sizeof(mem_block_t) + ALIGNMENT) {
        mem_block_t *new_block = (mem_block_t *)((char *)block + sizeof(mem_block_t) + size);
        new_block->size = block->size - size - sizeof(mem_block_t);
        
        new_block->free = 1;
        new_block->next = block->next;
        
        block->size = size;
        block->next = new_block;
    }
}


static void merge_free_blocks(void) {
    mem_block_t *current = free_list;
    while (current && current->next) {
        if (current->free && current->next->free) {
            current->size += sizeof(mem_block_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}


void *malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    size = ALIGN(size);
    
    mem_block_t *block = find_free_block(size);
    if (!block) {

        return NULL;
    }

    split_block(block, size);
    block->free = 0;

    return (char *)block + sizeof(mem_block_t);
}

// updated free
void free(void *ptr) {
    if (!ptr) return;
    
    if ((char *)ptr < memory_pool || (char *)ptr >= memory_pool + MEMORY_POOL_SIZE) {
        // Invalid pointer - just return silently in freestanding environment
        return;
    }

    mem_block_t *block = (mem_block_t *)((char *)ptr - sizeof(mem_block_t));
    block->free = 1;

    merge_free_blocks();
}


void *realloc(void *ptr, size_t new_size) {
    if (!ptr) {
        return malloc(new_size);
    }
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }

    mem_block_t *block = (mem_block_t *)((char *)ptr - sizeof(mem_block_t));
    if (block->size >= new_size) {
        split_block(block, ALIGN(new_size));
        return ptr;
    }


    void *new_ptr = malloc(new_size);
    if (!new_ptr) return NULL;


    memcpy(new_ptr, ptr, block->size < new_size ? block->size : new_size);
    free(ptr);
    return new_ptr;
}

void *calloc(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size;
    void *ptr = malloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void print_memory_state(void) {
    mem_block_t *current = free_list;
    kprint("Memory State:\n");
    while (current) {
        kprintf("Block %p - size: %zu, free: %d, next: %p\n",
               (void *)current, current->size, current->free, (void *)current->next);
        current = current->next;
    }
    kprint("\n");
}
