#ifndef PAUSE_ENGINE_H
#define PAUSE_ENGINE_H

#include <stdint.h>

/* Game Pause Injection — snapshot/restore (Johnny).
 * Intercept CPU loop, freeze state, dump to storage, resume.
 * Compatibility whitelist for safe games.
 * Reality: extremely difficult on real HW; see docs/REALITY_CHECK.md. */

#define PAUSE_SNAPSHOT_MAGIC  0x50534E50  /* "PSNP" */
#define PAUSE_PATH_MAX        128
#define PAUSE_WHITELIST_MAX   64

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t ee_ram_size;   /* bytes dumped */
    uint32_t iop_ram_size;
    uint32_t gs_state_size;
    char game_id[16];
    uint32_t checksum;
} pause_snapshot_header_t;

typedef enum {
    PAUSE_OK = 0,
    PAUSE_ERR_NOT_WHITELISTED,
    PAUSE_ERR_MEMORY,
    PAUSE_ERR_STORAGE,
    PAUSE_ERR_IO,
} pause_result_t;

/* Check if current title is whitelisted for pause/save-state. */
int pause_is_whitelisted(const char *game_id);

/* Add title to whitelist (by game ID or ELF name). */
int pause_whitelist_add(const char *game_id);

/* Save current execution state to path (file on storage). */
pause_result_t pause_snapshot_save(const char *path);

/* Restore state from path; returns PAUSE_OK on success. */
pause_result_t pause_snapshot_restore(const char *path);

/* Init: load whitelist, register with scheduler/game layer if needed. */
void pause_engine_init(void);

/* --- Workarounds: the "impossible" made possible --- */

/* Pause type for compatibility DB: none, trigger (write addr to open menu), checkpoint-only. */
typedef enum {
    PAUSE_TYPE_NONE = 0,
    PAUSE_TYPE_TRIGGER_ADDR,   /* write 1 to known address to open pause menu */
    PAUSE_TYPE_CHECKPOINT,     /* save only at safe points */
} pause_type_t;

typedef struct {
    char game_id[16];
    pause_type_t type;
    uint32_t trigger_addr;     /* for TRIGGER_ADDR: EE physical address */
    uint32_t trigger_value;   /* byte to write (often 1) */
} pause_compat_entry_t;

#define PAUSE_COMPAT_MAX  32

/* Register a game in the compatibility DB. */
int pause_compat_add(const char *game_id, pause_type_t type, uint32_t trigger_addr, uint32_t trigger_value);

/* Soft pause: try to open game's own pause menu (write trigger if in DB). */
pause_result_t pause_soft_trigger(const char *game_id);

/* Checkpoint save: save state at a "safe" point (caller decides when). */
pause_result_t pause_checkpoint_save(const char *game_id, const char *path);

/* Look up compat entry. */
int pause_compat_get(const char *game_id, pause_compat_entry_t *out);

#endif /* PAUSE_ENGINE_H */
