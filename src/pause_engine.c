/* Game Pause Injection — snapshot/restore (Johnny). */

#include "pause_engine.h"
#include "kernel.h"
#include <stddef.h>

#define WHITELIST_MAX  PAUSE_WHITELIST_MAX
static char whitelist[WHITELIST_MAX][16];
static unsigned int whitelist_count = 0;
static int inited = 0;

static int str_eq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == *b;
}

static void str_copy(char *dst, const char *src, unsigned int max) {
    unsigned int i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

void pause_engine_init(void) {
    if (inited) return;
    whitelist_count = 0;
    inited = 1;
}

int pause_is_whitelisted(const char *game_id) {
    if (!game_id) return 0;
    for (unsigned int i = 0; i < whitelist_count; i++) {
        if (str_eq(whitelist[i], game_id)) return 1;
    }
    return 0;
}

int pause_whitelist_add(const char *game_id) {
    if (!game_id || whitelist_count >= WHITELIST_MAX) return -1;
    str_copy(whitelist[whitelist_count], game_id, 16);
    whitelist_count++;
    return 0;
}

pause_result_t pause_snapshot_save(const char *path) {
    (void)path;
    kprint("  pause: snapshot save not yet implemented (EE/IOP/GS dump)\n");
    return PAUSE_ERR_IO;
}

pause_result_t pause_snapshot_restore(const char *path) {
    (void)path;
    kprint("  pause: snapshot restore not yet implemented\n");
    return PAUSE_ERR_IO;
}

/* --- Compatibility DB + soft pause + checkpoint --- */
static pause_compat_entry_t compat_db[PAUSE_COMPAT_MAX];
static unsigned int compat_count = 0;

int pause_compat_add(const char *game_id, pause_type_t type, uint32_t trigger_addr, uint32_t trigger_value) {
    if (!game_id || compat_count >= PAUSE_COMPAT_MAX) return -1;
    str_copy(compat_db[compat_count].game_id, game_id, 16);
    compat_db[compat_count].type = type;
    compat_db[compat_count].trigger_addr = trigger_addr;
    compat_db[compat_count].trigger_value = trigger_value;
    compat_count++;
    return 0;
}

int pause_compat_get(const char *game_id, pause_compat_entry_t *out) {
    if (!game_id || !out) return -1;
    for (unsigned int i = 0; i < compat_count; i++) {
        if (str_eq(compat_db[i].game_id, game_id)) {
            *out = compat_db[i];
            return 0;
        }
    }
    return -1;
}

pause_result_t pause_soft_trigger(const char *game_id) {
    pause_compat_entry_t e;
    if (pause_compat_get(game_id, &e) != 0) return PAUSE_ERR_NOT_WHITELISTED;
    if (e.type != PAUSE_TYPE_TRIGGER_ADDR) return PAUSE_ERR_NOT_WHITELISTED;
    /* Real: write e.trigger_value to EE address e.trigger_addr. Requires EE bus access. */
    volatile uint8_t *p = (volatile uint8_t *)e.trigger_addr;
    (void)p;
    /* *p = (uint8_t)e.trigger_value; */  /* enable when we have EE memory map */
    return PAUSE_OK;
}

pause_result_t pause_checkpoint_save(const char *game_id, const char *path) {
    (void)game_id;
    (void)path;
    /* Save state at "safe point" (e.g. when game returns to menu). Same as snapshot but only at known safe points. */
    return pause_snapshot_save(path);
}
