#include "music.h"
#include "kernel.h"
#include "fs.h"
#include <stddef.h>

static char now_playing[MUSIC_NOW_PLAYING_LEN];
static int background_mode;
static char playlist[MUSIC_PLAYLIST_MAX][MUSIC_PATH_LEN];
static int playlist_count;

void music_init(void) {
    now_playing[0] = '\0';
    background_mode = 0;
    playlist_count = 0;
    for (int i = 0; i < MUSIC_PLAYLIST_MAX; i++)
        playlist[i][0] = '\0';
}

static void copy_path(char *dst, const char *src, int max_len) {
    int i = 0;
    while (i < max_len - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

void music_play(const char *path_or_playlist) {
    if (!path_or_playlist || !path_or_playlist[0]) {
        if (now_playing[0])
            kprintf("Now playing: %s\n", now_playing);
        else
            kprint("No track selected. Use: music play <file>\n");
        return;
    }
    copy_path(now_playing, path_or_playlist, MUSIC_NOW_PLAYING_LEN);
    kprint("  ");
    kprint_color("music", 0x0A);
    kprintf(" Playing: %s\n", now_playing);
    kprint("  (Audio decode requires IOP/SPU2; playback simulated.)\n");
}

void music_list(const char *path) {
    (void)path;
    kprint("\n  ");
    kprint_color(" music list ", 0x2A);
    kprint_color(" (FAT12 root) -----------------\n", 0x08);
    fat12_list_files();
    kprint("  Use ");
    kprint_color("music play <file>", 0x0B);
    kprint(" to play. Supported: MP3, WAV.\n");
    kprint("  ----------------------------------------\n\n");
}

void music_background(int on) {
    background_mode = on ? 1 : 0;
    kprint("  ");
    kprint_color("music", 0x0A);
    kprintf(" background %s\n", background_mode ? "on" : "off");
}

int music_is_background(void) {
    return background_mode;
}

void music_print_status(void) {
    kprint("  ");
    kprint_color("music", 0x0A);
    kprintf(" status: %s  background: %s\n",
            now_playing[0] ? now_playing : "(none)",
            background_mode ? "on" : "off");
}

int music_add_to_playlist(const char *path) {
    if (!path || playlist_count >= MUSIC_PLAYLIST_MAX) return -1;
    copy_path(playlist[playlist_count], path, MUSIC_PATH_LEN);
    playlist_count++;
    return 0;
}

void music_playlist_clear(void) {
    playlist_count = 0;
    for (int i = 0; i < MUSIC_PLAYLIST_MAX; i++)
        playlist[i][0] = '\0';
}

void music_playlist_list(void) {
    kprint("\n  ");
    kprint_color(" playlist ", 0x2A);
    kprint_color(" -----------------------------\n", 0x08);
    if (playlist_count == 0) {
        kprint("  (empty)\n");
        kprint("  ----------------------------------------\n\n");
        return;
    }
    for (int i = 0; i < playlist_count; i++)
        kprintf("  %2d. %s\n", i + 1, playlist[i]);
    kprint("  ----------------------------------------\n\n");
}
