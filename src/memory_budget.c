/* RAM workaround: budget by region + optional swap to external storage. */

#include "memory_budget.h"
#include "kernel.h"
#include "memory_manager.h"
#include <stddef.h>

static budget_entry_t regions[BUDGET_REGION_MAX];
static swap_slot_t swap_slots[SWAP_SLOT_MAX];
static const char *region_names[] = {
    "kernel", "network", "fs", "streaming", "app", "pause_cache", "swap_cache", "other"
};
static int inited = 0;

#define DEFAULT_KERNEL_KB     (4 * 1024)
#define DEFAULT_NETWORK_KB    (2 * 1024)
#define DEFAULT_FS_KB         (1 * 1024)
#define DEFAULT_STREAMING_KB  (2 * 1024)
#define DEFAULT_APP_KB        (16 * 1024)
#define DEFAULT_PAUSE_KB      (2 * 1024)
#define DEFAULT_SWAP_KB       (4 * 1024)
#define DEFAULT_OTHER_KB      (1 * 1024)

void memory_budget_init(void) {
    if (inited) return;
    for (int i = 0; i < BUDGET_REGION_MAX; i++) {
        regions[i].limit_kb = 0;
        regions[i].used_kb = 0;
        regions[i].name = region_names[i];
    }
    memory_budget_set_limit(BUDGET_KERNEL, DEFAULT_KERNEL_KB);
    memory_budget_set_limit(BUDGET_NETWORK, DEFAULT_NETWORK_KB);
    memory_budget_set_limit(BUDGET_FS, DEFAULT_FS_KB);
    memory_budget_set_limit(BUDGET_STREAMING, DEFAULT_STREAMING_KB);
    memory_budget_set_limit(BUDGET_APP, DEFAULT_APP_KB);
    memory_budget_set_limit(BUDGET_PAUSE_CACHE, DEFAULT_PAUSE_KB);
    memory_budget_set_limit(BUDGET_SWAP_CACHE, DEFAULT_SWAP_KB);
    memory_budget_set_limit(BUDGET_OTHER, DEFAULT_OTHER_KB);
    for (int i = 0; i < SWAP_SLOT_MAX; i++) {
        swap_slots[i].in_use = 0;
        swap_slots[i].local_ptr = NULL;
    }
    inited = 1;
}

void memory_budget_set_limit(budget_region_t region, uint32_t limit_kb) {
    if ((unsigned)region >= BUDGET_REGION_MAX) return;
    regions[region].limit_kb = limit_kb;
}

int memory_budget_alloc(budget_region_t region, uint32_t size_bytes) {
    if (!inited) memory_budget_init();
    if ((unsigned)region >= BUDGET_REGION_MAX) return -1;
    uint32_t add_kb = (size_bytes + 1023) / 1024;
    if (regions[region].used_kb + add_kb > regions[region].limit_kb && regions[region].limit_kb != 0)
        return 0; /* over budget */
    regions[region].used_kb += add_kb;
    return 1;
}

void memory_budget_free(budget_region_t region, uint32_t size_bytes) {
    if ((unsigned)region >= BUDGET_REGION_MAX) return;
    uint32_t sub_kb = (size_bytes + 1023) / 1024;
    if (sub_kb > regions[region].used_kb) regions[region].used_kb = 0;
    else regions[region].used_kb -= sub_kb;
}

int memory_budget_can_alloc(budget_region_t region, uint32_t size_bytes) {
    if ((unsigned)region >= BUDGET_REGION_MAX) return 0;
    uint32_t add_kb = (size_bytes + 1023) / 1024;
    if (regions[region].limit_kb == 0) return 1;
    return (regions[region].used_kb + add_kb <= regions[region].limit_kb) ? 1 : 0;
}

uint32_t memory_budget_used_kb(budget_region_t region) {
    if ((unsigned)region >= BUDGET_REGION_MAX) return 0;
    return regions[region].used_kb;
}

uint32_t memory_budget_total_used_kb(void) {
    uint32_t t = 0;
    for (int i = 0; i < BUDGET_REGION_MAX; i++) t += regions[i].used_kb;
    return t;
}

int swap_slot_alloc(size_t size, budget_region_t region, swap_slot_t *out) {
    if (!out) return -1;
    void *p = malloc(size);
    if (!p) return -2;
    if (!memory_budget_alloc(BUDGET_SWAP_CACHE, (uint32_t)size)) {
        free(p);
        return -3; /* over budget */
    }
    for (int i = 0; i < SWAP_SLOT_MAX; i++) {
        if (!swap_slots[i].in_use) {
            swap_slots[i].in_use = 1;
            swap_slots[i].offset = (uint32_t)i;
            swap_slots[i].local_ptr = p;
            swap_slots[i].size = (uint32_t)size;
            swap_slots[i].region = region;
            *out = swap_slots[i];
            return 0;
        }
    }
    memory_budget_free(BUDGET_SWAP_CACHE, (uint32_t)size);
    free(p);
    return -4; /* no slot */
}

int swap_evict(swap_slot_t *slot, const char *swap_path) {
    (void)swap_path;
    if (!slot) return -1;
    if (slot->offset >= SWAP_SLOT_MAX) return -1;
    swap_slot_t *real = &swap_slots[slot->offset];
    if (!real->local_ptr) return 0; /* already evicted */
    /* Real impl: write real->local_ptr to swap file at offset slot->offset * SWAP_CHUNK_SIZE. */
    free(real->local_ptr);
    real->local_ptr = NULL;
    return 0;
}

int swap_restore(swap_slot_t *slot, const char *swap_path) {
    (void)swap_path;
    if (!slot || slot->offset >= SWAP_SLOT_MAX) return -1;
    swap_slot_t *real = &swap_slots[slot->offset];
    if (real->local_ptr) return 0; /* already in RAM */
    void *p = malloc(real->size);
    if (!p) return -2;
    real->local_ptr = p;
    slot->local_ptr = p;
    return 0;
}

void swap_slot_free(swap_slot_t *slot) {
    if (!slot || slot->offset >= SWAP_SLOT_MAX) return;
    swap_slot_t *real = &swap_slots[slot->offset];
    if (real->local_ptr) {
        free(real->local_ptr);
        memory_budget_free(BUDGET_SWAP_CACHE, real->size);
    }
    real->local_ptr = NULL;
    real->in_use = 0;
}

void swap_compress(const void *src, size_t len, void *dst, size_t *out_len) {
    /* Stub: copy only. Real: LZ77/RLE to fit more in swap file. */
    const unsigned char *s = (const unsigned char *)src;
    unsigned char *d = (unsigned char *)dst;
    size_t i = 0;
    while (i < len) { d[i] = s[i]; i++; }
    *out_len = len;
}

void swap_decompress(const void *src, size_t len, void *dst, size_t *out_len) {
    const unsigned char *s = (const unsigned char *)src;
    unsigned char *d = (unsigned char *)dst;
    size_t i = 0;
    while (i < len) { d[i] = s[i]; i++; }
    *out_len = len;
}
