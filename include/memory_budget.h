#ifndef MEMORY_BUDGET_H
#define MEMORY_BUDGET_H

#include <stdint.h>
#include <stddef.h>

/* RAM workaround: 32MB is fixed. We budget by region and optionally
 * evict to external storage (swap) so we never assume "infinite" RAM. */

#define BUDGET_REGION_MAX    8
#define SWAP_SLOT_MAX        32
#define SWAP_CHUNK_SIZE      4096   /* 4KB eviction unit */
#define PS2_RAM_BYTES        (32 * 1024 * 1024)

typedef enum {
    BUDGET_KERNEL = 0,
    BUDGET_NETWORK,
    BUDGET_FS,
    BUDGET_STREAMING,
    BUDGET_APP,
    BUDGET_PAUSE_CACHE,
    BUDGET_SWAP_CACHE,
    BUDGET_OTHER,
} budget_region_t;

typedef struct {
    uint32_t limit_kb;      /* max KB for this region */
    uint32_t used_kb;       /* current usage */
    const char *name;
} budget_entry_t;

typedef struct {
    int in_use;
    uint32_t offset;        /* offset in swap file (chunk index) */
    void *local_ptr;       /* when swapped in */
    uint32_t size;
    budget_region_t region;
} swap_slot_t;

/* Init: set default limits (kernel 4MB, network 2MB, app 16MB, etc.). */
void memory_budget_init(void);

/* Set limit for a region (in KB). */
void memory_budget_set_limit(budget_region_t region, uint32_t limit_kb);

/* Track allocation: call after malloc for a region. Returns 0 if over budget. */
int memory_budget_alloc(budget_region_t region, uint32_t size_bytes);

/* Track free: call before free. */
void memory_budget_free(budget_region_t region, uint32_t size_bytes);

/* Check if allocating size_bytes in region would stay within budget. */
int memory_budget_can_alloc(budget_region_t region, uint32_t size_bytes);

/* Get usage for a region or total (in KB). */
uint32_t memory_budget_used_kb(budget_region_t region);
uint32_t memory_budget_total_used_kb(void);

/* Swap layer: reserve a slot that can be evicted to storage. */
int swap_slot_alloc(size_t size, budget_region_t region, swap_slot_t *out);

/* Evict slot to storage (write to path). Call when low on RAM. */
int swap_evict(swap_slot_t *slot, const char *swap_path);

/* Restore slot from storage (read back). */
int swap_restore(swap_slot_t *slot, const char *swap_path);

/* Free slot (and its backing file chunk if any). */
void swap_slot_free(swap_slot_t *slot);

/* Optional: compress buffer before swap (stub: copy only; real impl = LZ/RLE). */
void swap_compress(const void *src, size_t len, void *dst, size_t *out_len);
void swap_decompress(const void *src, size_t len, void *dst, size_t *out_len);

#endif /* MEMORY_BUDGET_H */
