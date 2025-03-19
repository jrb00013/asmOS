#include "memory_manager.h"
#include <stdio.h>
#include <stdint.h>

static char memory_pool[MEMORY_POOL_SIZE];
static mem_block_t *free_list = NULL;

// Function to initialize memory pool
void init_memory_manager(void) {
    free_list = (mem_block_t *)memory_pool;
    free_list->size = MEMORY_POOL_SIZE - sizeof(mem_block_t);
    free_list->next = NULL;
    free_list->free = 1;
}

// Function to find a free block using first-fit strategy
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

// Function to allocate memory dynamically
void *malloc(size_t size) {
    if (size == 0) {
        return NULL; // Invalid request
    }

    mem_block_t *block = find_free_block(size);

    if (!block) {
        return NULL; // No suitable block found
    }

    // Mark block as allocated
    block->free = 0;

    // Split the block if there's excess space
    if (block->size > size + sizeof(mem_block_t)) {
        mem_block_t *new_block = (mem_block_t *)((char *)block + sizeof(mem_block_t) + size);
        new_block->size = block->size - size - sizeof(mem_block_t);
        new_block->free = 1;
        new_block->next = block->next;
        block->next = new_block;
        block->size = size;
    }

    return (char *)block + sizeof(mem_block_t);
}

// Function to free allocated memory
void free(void *ptr) {
    if (!ptr) return; // Null check

    mem_block_t *block = (mem_block_t *)((char *)ptr - sizeof(mem_block_t));
    block->free = 1;

    // Try to merge adjacent free blocks
    mem_block_t *current = free_list;
    while (current) {
        if (current->free && current->next && current->next->free) {
            current->size += current->next->size + sizeof(mem_block_t);
            current->next = current->next->next;
        }
        current = current->next;
    }
}
