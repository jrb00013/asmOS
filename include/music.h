#ifndef MUSIC_H
#define MUSIC_H

#define MUSIC_PLAYLIST_MAX   32
#define MUSIC_PATH_LEN       64
#define MUSIC_NOW_PLAYING_LEN 80

void music_init(void);
void music_play(const char *path_or_playlist);
void music_list(const char *path);
void music_background(int on);
int music_is_background(void);
void music_print_status(void);
int music_add_to_playlist(const char *path);
void music_playlist_clear(void);
void music_playlist_list(void);

#endif
