#include "game_history.h"
#include "kernel.h"
#include <stddef.h>

static game_history_entry_t entries[GAME_HISTORY_MAX_ENTRIES];
static int entry_count;
static int current_session_game;
static uint32_t session_start_tick;
static uint32_t system_tick;
static uint32_t last_session_id;

void game_history_tick(void) {
    system_tick++;
}

uint32_t game_history_get_tick(void) {
    return system_tick;
}

static void format_date(char *buf, unsigned int len) {
    (void)len;
    buf[0] = 'S';
    buf[1] = 'e';
    buf[2] = 's';
    buf[3] = ' ';
    buf[4] = '0' + (last_session_id / 10) % 10;
    buf[5] = '0' + last_session_id % 10;
    buf[6] = '\0';
}

void game_history_init(void) {
    entry_count = 0;
    current_session_game = -1;
    session_start_tick = 0;
    system_tick = 0;
    last_session_id = 0;
    for (int i = 0; i < GAME_HISTORY_MAX_ENTRIES; i++) {
        entries[i].game_index = -1;
        entries[i].name[0] = '\0';
        entries[i].playtime_sec = 0;
        entries[i].last_played[0] = '\0';
        entries[i].launch_count = 0;
        entries[i].last_session_id = 0;
    }
}

void game_history_session_start(int game_index, const char *name) {
    current_session_game = game_index;
    session_start_tick = system_tick;
    (void)name;
}

void game_history_session_end(int game_index, const char *name) {
    last_session_id++;
    uint32_t elapsed = system_tick - session_start_tick;
    if (elapsed < 1) elapsed = 1;
    current_session_game = -1;

    int found = -1;
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].game_index == game_index) {
            found = i;
            break;
        }
    }
    if (found >= 0) {
        entries[found].playtime_sec += elapsed;
        entries[found].launch_count++;
        entries[found].last_session_id = last_session_id;
        format_date(entries[found].last_played, GAME_HISTORY_DATE_LEN);
        return;
    }
    if (entry_count >= GAME_HISTORY_MAX_ENTRIES)
        return;
    int i = entry_count++;
    entries[i].game_index = game_index;
    entries[i].playtime_sec = elapsed;
    entries[i].launch_count = 1;
    entries[i].last_session_id = last_session_id;
    format_date(entries[i].last_played, GAME_HISTORY_DATE_LEN);
    int j = 0;
    while (j < GAME_NAME_LEN - 1 && name[j]) {
        entries[i].name[j] = name[j];
        j++;
    }
    entries[i].name[j] = '\0';
}

int game_history_get_entry(int game_index, game_history_entry_t *out) {
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].game_index == game_index) {
            if (out) *out = entries[i];
            return 1;
        }
    }
    return 0;
}

void game_history_print_history(void) {
    kprint("\n  ");
    kprint_color(" games history ", 0x2A);
    kprint_color(" --------------------------------\n", 0x08);
    if (entry_count == 0) {
        kprint("  No games played yet. Use ");
        kprint_color("game <n>", 0x0B);
        kprint(" to play.\n");
        kprint("  ----------------------------------------\n\n");
        return;
    }
    for (int i = 0; i < entry_count; i++) {
        kprintf("  %-12s  last: %-8s  playtime: %u ticks  launches: %u\n",
                entries[i].name, entries[i].last_played,
                (unsigned)entries[i].playtime_sec, (unsigned)entries[i].launch_count);
    }
    kprint("  ----------------------------------------\n\n");
}

void game_history_print_stats(void) {
    kprint("\n  ");
    kprint_color(" games stats ", 0x2A);
    kprint_color(" ----------------------------------\n", 0x08);
    if (entry_count == 0) {
        kprint("  No games played yet.\n");
        kprint("  ----------------------------------------\n\n");
        return;
    }
    uint32_t total_playtime = 0;
    uint32_t total_launches = 0;
    int most_played_idx = 0;
    for (int i = 0; i < entry_count; i++) {
        total_playtime += entries[i].playtime_sec;
        total_launches += entries[i].launch_count;
        if (entries[i].playtime_sec > entries[most_played_idx].playtime_sec)
            most_played_idx = i;
    }
    kprintf("  Total playtime:  %u ticks\n", (unsigned)total_playtime);
    kprintf("  Total launches: %u\n", (unsigned)total_launches);
    kprintf("  Most played:    %s (%u ticks)\n",
            entries[most_played_idx].name, (unsigned)entries[most_played_idx].playtime_sec);
    kprint("  ----------------------------------------\n\n");
}

void game_history_print_last(void) {
    kprint("\n  ");
    kprint_color(" games last ", 0x2A);
    kprint_color(" -------------------------------------\n", 0x08);
    if (entry_count == 0) {
        kprint("  No games played yet.\n");
        kprint("  ----------------------------------------\n\n");
        return;
    }
    int last_idx = 0;
    for (int i = 1; i < entry_count; i++) {
        if (entries[i].last_session_id > entries[last_idx].last_session_id)
            last_idx = i;
    }
    kprintf("  Last played: %s\n", entries[last_idx].name);
    kprintf("  Session:     %s  |  playtime: %u ticks  |  launches: %u\n",
            entries[last_idx].last_played, (unsigned)entries[last_idx].playtime_sec,
            (unsigned)entries[last_idx].launch_count);
    kprint("  ----------------------------------------\n\n");
}
