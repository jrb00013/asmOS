#ifndef GAME_H
#define GAME_H

#include <stdint.h>

// Game types
#define GAME_TYPE_ACTION    1
#define GAME_TYPE_PUZZLE   2
#define GAME_TYPE_RACING   3
#define GAME_TYPE_RPG      4
#define GAME_TYPE_SHOOTER  5

// Game information structure
typedef struct {
    char name[32];
    char description[128];
    uint32_t size;
    uint32_t type;
    void (*init_func)(void);
    void (*run_func)(void);
    void (*cleanup_func)(void);
} game_info_t;

// Game system functions
void init_game_system(void);
void list_games(void);
void launch_game(int game_index);
void stop_game(void);
const char* get_game_type_name(uint32_t type);
game_info_t* get_current_game(void);
int is_game_running(void);

// Individual game functions
void snake_init(void);
void snake_run(void);
void snake_cleanup(void);

void pong_init(void);
void pong_run(void);
void pong_cleanup(void);

void tetris_init(void);
void tetris_run(void);
void tetris_cleanup(void);

void space_invaders_init(void);
void space_invaders_run(void);
void space_invaders_cleanup(void);

void racing_init(void);
void racing_run(void);
void racing_cleanup(void);

#endif // GAME_H
