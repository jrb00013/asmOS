#ifndef GAME_HISTORY_H
#define GAME_HISTORY_H

#include <stdint.h>

#define GAME_HISTORY_MAX_ENTRIES  64
#define GAME_NAME_LEN            32
#define GAME_HISTORY_DATE_LEN    16

typedef struct {
    int game_index;
    char name[GAME_NAME_LEN];
    uint32_t playtime_sec;
    char last_played[GAME_HISTORY_DATE_LEN];
    uint32_t launch_count;
    uint32_t last_session_id;
} game_history_entry_t;

void game_history_init(void);
void game_history_tick(void);
uint32_t game_history_get_tick(void);
void game_history_session_start(int game_index, const char *name);
void game_history_session_end(int game_index, const char *name);
void game_history_print_history(void);
void game_history_print_stats(void);
void game_history_print_last(void);
int game_history_get_entry(int game_index, game_history_entry_t *out);

#endif
