#include "game.h"
#include "game_history.h"
#include "graphics.h"
#include "kernel.h"
#include "platform.h"
#include "video.h"
#include "keyboard.h"
#include <stdint.h>

// Game system for PS2
// Game types
#define GAME_TYPE_ACTION    1
#define GAME_TYPE_PUZZLE   2
#define GAME_TYPE_RACING   3
#define GAME_TYPE_RPG      4
#define GAME_TYPE_SHOOTER  5

// Available games
static game_info_t games[] = {
    {
        "Snake",
        "Classic snake game - eat food and grow longer",
        1024,
        GAME_TYPE_PUZZLE,
        snake_init,
        snake_run,
        snake_cleanup
    },
    {
        "Pong",
        "Classic pong game - bounce the ball",
        512,
        GAME_TYPE_ACTION,
        pong_init,
        pong_run,
        pong_cleanup
    },
    {
        "Tetris",
        "Classic tetris - arrange falling blocks",
        2048,
        GAME_TYPE_PUZZLE,
        tetris_init,
        tetris_run,
        tetris_cleanup
    },
    {
        "Space Invaders",
        "Classic space invaders - shoot aliens",
        1536,
        GAME_TYPE_SHOOTER,
        space_invaders_init,
        space_invaders_run,
        space_invaders_cleanup
    },
    {
        "Racing",
        "Simple racing game - avoid obstacles",
        1024,
        GAME_TYPE_RACING,
        racing_init,
        racing_run,
        racing_cleanup
    }
};

static int game_count = sizeof(games) / sizeof(games[0]);
static int current_game = -1;
static int game_running = 0;

// Initialize game system
void init_game_system(void) {
    kprint("Initializing PS2 Game System...\n");
    
    // Initialize graphics
    init_graphics_demo();
    
    kprintf("Game system initialized! %d games available\n", game_count);
}

// List available games
void list_games(void) {
    kprint("Available Games:\n");
    kprint("================\n");
    
    for (int i = 0; i < game_count; i++) {
        kprintf("%d. %s (%s)\n", i + 1, games[i].name, games[i].description);
        kprintf("   Type: %s, Size: %u KB\n", get_game_type_name(games[i].type), games[i].size);
    }
}

void launch_game(int game_index) {
    if (game_index < 0 || game_index >= game_count) {
        kprint("Invalid game index!\n");
        return;
    }

    current_game = game_index;
    game_info_t* game = &games[game_index];

    game_history_session_start(game_index, game->name);
    kprintf("Launching %s...\n", game->name);

#ifndef PS2_HARDWARE
    plat_video_mode_13h();
    graphics_set_palette();
#else
    plat_video_mode_13h();
    graphics_set_palette();
#endif

    if (game->init_func)
        game->init_func();

    game_running = 1;
    if (game->run_func)
        game->run_func();
    if (game->cleanup_func)
        game->cleanup_func();

    game_running = 0;
    game_history_session_end(game_index, game->name);
    current_game = -1;

#ifndef PS2_HARDWARE
    plat_video_mode_text();
#else
    plat_video_mode_text();
#endif
    kprintf("%s completed!\n", game->name);
}

// Get game type name
const char* get_game_type_name(uint32_t type) {
    switch (type) {
        case GAME_TYPE_ACTION: return "Action";
        case GAME_TYPE_PUZZLE: return "Puzzle";
        case GAME_TYPE_RACING: return "Racing";
        case GAME_TYPE_RPG: return "RPG";
        case GAME_TYPE_SHOOTER: return "Shooter";
        default: return "Unknown";
    }
}

/* Snake: grid 16x10, cell 20x20 pixels. Max length 64. */
#define SNAKE_GRID_W   16
#define SNAKE_GRID_H   10
#define SNAKE_CELL     20
#define SNAKE_MAX_LEN  64
#define SNAKE_UP       0
#define SNAKE_RIGHT    1
#define SNAKE_DOWN     2
#define SNAKE_LEFT     3

static int snake_seg_x[SNAKE_MAX_LEN];
static int snake_seg_y[SNAKE_MAX_LEN];
static int snake_len;
static int snake_dir;
static int snake_next_dir;
static int snake_food_x;
static int snake_food_y;
static int snake_game_over;
static int snake_score;

static const int snake_dx[] = { 0, 1, 0, -1 };
static const int snake_dy[] = { -1, 0, 1, 0 };

static void snake_place_food(void) {
    int n = 0;
    int x, y;
    do {
        x = 1 + (snake_score * 7 + n) % (SNAKE_GRID_W - 2);
        y = 1 + (snake_score * 11 + n * 3) % (SNAKE_GRID_H - 2);
        n++;
        for (int i = 0; i < snake_len; i++)
            if (snake_seg_x[i] == x && snake_seg_y[i] == y) { x = -1; break; }
    } while (x <= 0 && n < 200);
    if (x <= 0) x = 1, y = 1;
    snake_food_x = x;
    snake_food_y = y;
}

void snake_init(void) {
    snake_len = 3;
    snake_dir = SNAKE_RIGHT;
    snake_next_dir = SNAKE_RIGHT;
    snake_score = 0;
    snake_game_over = 0;
    snake_seg_x[0] = 8; snake_seg_y[0] = 5;
    snake_seg_x[1] = 7; snake_seg_y[1] = 5;
    snake_seg_x[2] = 6; snake_seg_y[2] = 5;
    snake_place_food();
}

static void snake_tick(void) {
    int opp = (snake_dir + 2) % 4;
    if (snake_next_dir != opp)
        snake_dir = snake_next_dir;

    int hx = snake_seg_x[0] + snake_dx[snake_dir];
    int hy = snake_seg_y[0] + snake_dy[snake_dir];

    if (hx <= 0 || hx >= SNAKE_GRID_W - 1 || hy <= 0 || hy >= SNAKE_GRID_H - 1) {
        snake_game_over = 1;
        return;
    }
    for (int i = 0; i < snake_len; i++)
        if (snake_seg_x[i] == hx && snake_seg_y[i] == hy) {
            snake_game_over = 1;
            return;
        }

    for (int i = snake_len; i > 0; i--) {
        snake_seg_x[i] = snake_seg_x[i - 1];
        snake_seg_y[i] = snake_seg_y[i - 1];
    }
    snake_seg_x[0] = hx;
    snake_seg_y[0] = hy;

    if (hx == snake_food_x && hy == snake_food_y) {
        if (snake_len < SNAKE_MAX_LEN - 1) snake_len++;
        snake_score++;
        snake_place_food();
    }
}

static void snake_draw(void) {
    clear_graphics_screen();

    for (int i = 0; i < snake_len; i++) {
        uint32_t cx = (uint32_t)(snake_seg_x[i] * SNAKE_CELL + 1);
        uint32_t cy = (uint32_t)(snake_seg_y[i] * SNAKE_CELL + 1);
        draw_rectangle(cx, cy, SNAKE_CELL - 2, SNAKE_CELL - 2, i == 0 ? 0x00FF00 : 0x00AA00);
    }
    {
        uint32_t fx = (uint32_t)(snake_food_x * SNAKE_CELL + 1);
        uint32_t fy = (uint32_t)(snake_food_y * SNAKE_CELL + 1);
        draw_rectangle(fx, fy, SNAKE_CELL - 2, SNAKE_CELL - 2, 0xFF0000);
    }

    draw_text(5, 2, "Snake", 0xFFFFFF);
    draw_text(5, 12, "Score:", 0xFFFFFF);
    draw_number(50, 12, (uint32_t)snake_score, 0xFFFFFF);
}

void snake_run(void) {
    while (!snake_game_over) {
        uint8_t sc;
        while ((sc = keyboard_get_scancode()) != 0) {
            if (sc & 0x80) continue;
            if (sc == 0x48) snake_next_dir = SNAKE_UP;
            else if (sc == 0x50) snake_next_dir = SNAKE_DOWN;
            else if (sc == 0x4B) snake_next_dir = SNAKE_LEFT;
            else if (sc == 0x4D) snake_next_dir = SNAKE_RIGHT;
        }
        snake_tick();
        game_history_tick();
        if (snake_game_over) break;
        snake_draw();
        for (volatile int d = 0; d < 80000; d++) ;
    }

    snake_draw();
    draw_text(80, 90, "Game Over", 0xFFFFFF);
    draw_text(60, 105, "Score:", 0xFFFFFF);
    draw_number(110, 105, (uint32_t)snake_score, 0xFFFFFF);
    draw_text(50, 125, "Press any key", 0xAAAAAA);
    while (!keyboard_has_key()) { for (volatile int d = 0; d < 5000; d++) ; }
    while (keyboard_get_scancode() != 0) ;
}

void snake_cleanup(void) {
    (void)0;
}

/* Pong: paddles left/right, ball, score. Player left (W/S), AI right. */
#define PONG_PAD_W     6
#define PONG_PAD_H     28
#define PONG_PAD_X_L   12
#define PONG_PAD_X_R   (VIDEO_WIDTH - 12 - PONG_PAD_W)
#define PONG_BALL_SZ   6
#define PONG_TOP       25
#define PONG_BOT       (VIDEO_HEIGHT - 5)
#define PONG_SPEED     2

static int pong_ly, pong_ry;   /* paddle center y */
static int pong_bx, pong_by;
static int pong_vx, pong_vy;
static int pong_score_l, pong_score_r;
static int pong_quit;

void pong_init(void) {
    pong_ly = pong_ry = VIDEO_HEIGHT / 2 - PONG_PAD_H / 2;
    pong_bx = VIDEO_WIDTH / 2 - PONG_BALL_SZ / 2;
    pong_by = VIDEO_HEIGHT / 2 - PONG_BALL_SZ / 2;
    pong_vx = PONG_SPEED;
    pong_vy = 1;
    pong_score_l = pong_score_r = 0;
    pong_quit = 0;
}

static void pong_tick(void) {
    pong_bx += pong_vx;
    pong_by += pong_vy;

    if (pong_by <= PONG_TOP) { pong_by = PONG_TOP; pong_vy = -pong_vy; }
    if (pong_by + PONG_BALL_SZ >= PONG_BOT) { pong_by = PONG_BOT - PONG_BALL_SZ; pong_vy = -pong_vy; }

    if (pong_bx <= PONG_PAD_X_L + PONG_PAD_W && pong_by + PONG_BALL_SZ >= pong_ly && pong_by <= pong_ly + PONG_PAD_H) {
        pong_bx = PONG_PAD_X_L + PONG_PAD_W;
        pong_vx = -pong_vx;
    }
    if (pong_bx + PONG_BALL_SZ >= PONG_PAD_X_R && pong_by + PONG_BALL_SZ >= pong_ry && pong_by <= pong_ry + PONG_PAD_H) {
        pong_bx = PONG_PAD_X_R - PONG_BALL_SZ;
        pong_vx = -pong_vx;
    }

    if (pong_bx < 0) { pong_score_r++; pong_bx = VIDEO_WIDTH / 2 - PONG_BALL_SZ / 2; pong_by = VIDEO_HEIGHT / 2 - PONG_BALL_SZ / 2; pong_vx = PONG_SPEED; pong_vy = 1; }
    if (pong_bx + PONG_BALL_SZ > VIDEO_WIDTH) { pong_score_l++; pong_bx = VIDEO_WIDTH / 2 - PONG_BALL_SZ / 2; pong_by = VIDEO_HEIGHT / 2 - PONG_BALL_SZ / 2; pong_vx = -PONG_SPEED; pong_vy = 1; }

    /* AI: follow ball */
    if (pong_ry + PONG_PAD_H / 2 < pong_by + PONG_BALL_SZ / 2 && pong_ry + PONG_PAD_H < PONG_BOT - 2) pong_ry += 2;
    if (pong_ry + PONG_PAD_H / 2 > pong_by + PONG_BALL_SZ / 2 && pong_ry > PONG_TOP + 2) pong_ry -= 2;
}

static void pong_draw(void) {
    clear_graphics_screen();
    draw_rectangle(PONG_PAD_X_L, pong_ly, PONG_PAD_W, PONG_PAD_H, 0xFFFFFF);
    draw_rectangle(PONG_PAD_X_R, pong_ry, PONG_PAD_W, PONG_PAD_H, 0xFFFFFF);
    draw_rectangle(pong_bx, pong_by, PONG_BALL_SZ, PONG_BALL_SZ, 0x00FF00);
    draw_text(5, 5, "Pong", 0xFFFFFF);
    draw_text(80, 5, "L:", 0xAAAAAA);
    draw_number(95, 5, (uint32_t)pong_score_l, 0xFFFFFF);
    draw_text(140, 5, "R:", 0xAAAAAA);
    draw_number(155, 5, (uint32_t)pong_score_r, 0xFFFFFF);
    draw_text(100, 190, "W/S paddle  Esc quit", 0x666666);
}

void pong_run(void) {
    while (!pong_quit) {
        uint8_t sc;
        while ((sc = keyboard_get_scancode()) != 0) {
            if (sc & 0x80) continue;
            if (sc == 0x01) { pong_quit = 1; break; }
            if (sc == 0x11) { if (pong_ly > PONG_TOP + 2) pong_ly -= 8; }
            if (sc == 0x1F) { if (pong_ly + PONG_PAD_H < PONG_BOT - 2) pong_ly += 8; }
            if (sc == 0x48) { if (pong_ly > PONG_TOP + 2) pong_ly -= 8; }
            if (sc == 0x50) { if (pong_ly + PONG_PAD_H < PONG_BOT - 2) pong_ly += 8; }
        }
        pong_tick();
        game_history_tick();
        pong_draw();
        for (volatile int d = 0; d < 40000; d++) ;
    }
    draw_text(100, 95, "Press any key", 0xAAAAAA);
    while (!keyboard_has_key()) { for (volatile int d = 0; d < 5000; d++) ; }
    while (keyboard_get_scancode() != 0) ;
}

void pong_cleanup(void) {
    (void)0;
}

/* Tetris: 10x20 board, 7 tetrominoes. Left/right/down, rotate. Line clear. */
#define TETRIS_W       10
#define TETRIS_H       20
#define TETRIS_CELL    7
#define TETRIS_OX      45
#define TETRIS_OY      15
#define TETRIS_NUM     7

/* 7 shapes, 4 rotations each, 4 cells (x,y in 0..3) */
static const int8_t tetris_shapes[7][4][4][2] = {
    { {{1,0},{1,1},{1,2},{1,3}}, {{0,1},{1,1},{2,1},{3,1}}, {{1,0},{1,1},{1,2},{1,3}}, {{0,1},{1,1},{2,1},{3,1}} },
    { {{0,0},{1,0},{0,1},{1,1}}, {{0,0},{1,0},{0,1},{1,1}}, {{0,0},{1,0},{0,1},{1,1}}, {{0,0},{1,0},{0,1},{1,1}} },
    { {{1,0},{0,1},{1,1},{2,1}}, {{0,0},{0,1},{1,1},{0,2}}, {{0,1},{1,1},{2,1},{1,2}}, {{1,0},{0,1},{1,1},{1,2}} },
    { {{1,0},{2,0},{0,1},{1,1}}, {{0,0},{0,1},{1,1},{1,2}}, {{1,0},{2,0},{0,1},{1,1}}, {{0,0},{0,1},{1,1},{1,2}} },
    { {{0,0},{1,0},{1,1},{2,1}}, {{1,0},{0,1},{1,1},{0,2}}, {{0,0},{1,0},{1,1},{2,1}}, {{1,0},{0,1},{1,1},{0,2}} },
    { {{0,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{0,2},{1,2}}, {{0,1},{1,1},{2,1},{2,2}}, {{0,0},{1,0},{0,1},{0,2}} },
    { {{2,0},{0,1},{1,1},{2,1}}, {{0,0},{1,0},{0,1},{0,2}}, {{0,1},{1,1},{2,1},{0,2}}, {{1,0},{1,1},{0,2},{1,2}} },
};
static const uint32_t tetris_colors[] = { 0x00FFFF, 0xFFFF00, 0xFF00FF, 0x00FF00, 0xFF0000, 0x0000FF, 0xFF8800 };

static uint8_t tetris_board[TETRIS_W][TETRIS_H];
static int tetris_piece, tetris_rot, tetris_px, tetris_py;
static int tetris_score, tetris_lines, tetris_quit;
static int tetris_drop_ticks;
#define TETRIS_DROP_MAX 25

static int tetris_collide(int dx, int dy, int rot) {
    for (int i = 0; i < 4; i++) {
        int cx = tetris_px + tetris_shapes[tetris_piece][rot][i][0] + dx;
        int cy = tetris_py + tetris_shapes[tetris_piece][rot][i][1] + dy;
        if (cx < 0 || cx >= TETRIS_W || cy >= TETRIS_H) return 1;
        if (cy >= 0 && tetris_board[cx][cy]) return 1;
    }
    return 0;
}

static void tetris_lock(void) {
    for (int i = 0; i < 4; i++) {
        int cx = tetris_px + tetris_shapes[tetris_piece][tetris_rot][i][0];
        int cy = tetris_py + tetris_shapes[tetris_piece][tetris_rot][i][1];
        if (cy >= 0 && cy < TETRIS_H) tetris_board[cx][cy] = (uint8_t)(tetris_piece + 1);
    }
    int cleared = 0;
    for (int row = TETRIS_H - 1; row >= 0; row--) {
        int full = 1;
        for (int c = 0; c < TETRIS_W; c++) if (!tetris_board[c][row]) { full = 0; break; }
        if (full) {
            cleared++;
            for (int r = row; r > 0; r--)
                for (int c = 0; c < TETRIS_W; c++) tetris_board[c][r] = tetris_board[c][r-1];
            for (int c = 0; c < TETRIS_W; c++) tetris_board[c][0] = 0;
            row++;
        }
    }
    tetris_lines += cleared;
    tetris_score += cleared * cleared * 100;
    tetris_piece = tetris_score % TETRIS_NUM;
    tetris_rot = 0;
    tetris_px = TETRIS_W / 2 - 2;
    tetris_py = -2;
    tetris_drop_ticks = 0;
    if (tetris_collide(0, 0, 0)) tetris_quit = 1;
}

void tetris_init(void) {
    for (int x = 0; x < TETRIS_W; x++)
        for (int y = 0; y < TETRIS_H; y++)
            tetris_board[x][y] = 0;
    tetris_piece = 0;
    tetris_rot = 0;
    tetris_px = TETRIS_W / 2 - 2;
    tetris_py = -2;
    tetris_score = tetris_lines = 0;
    tetris_quit = 0;
    tetris_drop_ticks = 0;
}

static void tetris_draw(void) {
    clear_graphics_screen();
    for (int x = 0; x < TETRIS_W; x++)
        for (int y = 0; y < TETRIS_H; y++) {
            if (!tetris_board[x][y]) continue;
            uint32_t c = tetris_colors[tetris_board[x][y] - 1];
            draw_rectangle(TETRIS_OX + x * TETRIS_CELL, TETRIS_OY + y * TETRIS_CELL, TETRIS_CELL - 1, TETRIS_CELL - 1, c);
        }
    for (int i = 0; i < 4; i++) {
        int cx = tetris_px + tetris_shapes[tetris_piece][tetris_rot][i][0];
        int cy = tetris_py + tetris_shapes[tetris_piece][tetris_rot][i][1];
        if (cy >= 0)
            draw_rectangle(TETRIS_OX + cx * TETRIS_CELL, TETRIS_OY + cy * TETRIS_CELL, TETRIS_CELL - 1, TETRIS_CELL - 1, tetris_colors[tetris_piece]);
    }
    draw_text(5, 5, "Tetris", 0xFFFFFF);
    draw_text(5, 15, "Score:", 0xAAAAAA);
    draw_number(45, 15, (uint32_t)tetris_score, 0xFFFFFF);
    draw_text(5, 25, "Lines:", 0xAAAAAA);
    draw_number(45, 25, (uint32_t)tetris_lines, 0xFFFFFF);
    draw_text(125, 5, "Arrows move", 0x666666);
    draw_text(125, 15, "Up rotate", 0x666666);
    draw_text(125, 25, "Esc quit", 0x666666);
}

void tetris_run(void) {
    while (!tetris_quit) {
        uint8_t sc;
        while ((sc = keyboard_get_scancode()) != 0) {
            if (sc & 0x80) continue;
            if (sc == 0x01) { tetris_quit = 1; break; }
            if (sc == 0x4B && !tetris_collide(-1, 0, tetris_rot)) tetris_px--;
            if (sc == 0x4D && !tetris_collide(1, 0, tetris_rot)) tetris_px++;
            if (sc == 0x50) { if (!tetris_collide(0, 1, tetris_rot)) tetris_py++; else tetris_lock(); }
            if (sc == 0x48) { int r = (tetris_rot + 1) % 4; if (!tetris_collide(0, 0, r)) tetris_rot = r; }
        }
        tetris_drop_ticks++;
        game_history_tick();
        if (tetris_drop_ticks >= TETRIS_DROP_MAX) {
            tetris_drop_ticks = 0;
            if (!tetris_collide(0, 1, tetris_rot)) tetris_py++;
            else tetris_lock();
        }
        tetris_draw();
        for (volatile int d = 0; d < 12000; d++) ;
    }
    tetris_draw();
    draw_text(95, 80, "Game Over", 0xFFFFFF);
    draw_text(80, 95, "Score:", 0xAAAAAA);
    draw_number(130, 95, (uint32_t)tetris_score, 0xFFFFFF);
    draw_text(70, 120, "Press any key", 0xAAAAAA);
    while (!keyboard_has_key()) { for (volatile int d = 0; d < 5000; d++) ; }
    while (keyboard_get_scancode() != 0) ;
}

void tetris_cleanup(void) {
    (void)0;
}

/* Space Invaders: player at bottom, grid of aliens, shoot. */
#define SI_ALIEN_COLS   8
#define SI_ALIEN_ROWS   3
#define SI_ALIEN_W      14
#define SI_ALIEN_H      10
#define SI_ALIEN_OX     20
#define SI_ALIEN_OY     20
#define SI_PLAYER_W     20
#define SI_PLAYER_H     12
#define SI_BULLET_W     4
#define SI_BULLET_H     8
#define SI_BULLET_SPD   4
#define SI_ALIEN_SPD    2

static int si_px;
static int si_alien_dx;
static int si_alien_ox;
static int si_alien_oy;
static int si_alien_tick;
static uint8_t si_aliens[SI_ALIEN_COLS][SI_ALIEN_ROWS];
static int si_bullet_x, si_bullet_y;
static int si_bullet_active;
static int si_score, si_quit, si_win;

void space_invaders_init(void) {
    si_px = VIDEO_WIDTH / 2 - SI_PLAYER_W / 2;
    si_alien_dx = SI_ALIEN_SPD;
    si_alien_ox = 0;
    si_alien_oy = 0;
    si_alien_tick = 0;
    for (int c = 0; c < SI_ALIEN_COLS; c++)
        for (int r = 0; r < SI_ALIEN_ROWS; r++)
            si_aliens[c][r] = 1;
    si_bullet_active = 0;
    si_score = 0;
    si_quit = 0;
    si_win = 0;
}

static void si_fire(void) {
    if (si_bullet_active) return;
    si_bullet_x = si_px + SI_PLAYER_W / 2 - SI_BULLET_W / 2;
    si_bullet_y = VIDEO_HEIGHT - SI_PLAYER_H - SI_BULLET_H;
    si_bullet_active = 1;
}

static void space_invaders_tick(void) {
    si_bullet_y -= SI_BULLET_SPD;
    if (si_bullet_y < 0) si_bullet_active = 0;
    if (si_bullet_active) {
        int bx = si_bullet_x + SI_BULLET_W / 2;
        int by = si_bullet_y + SI_BULLET_H / 2;
        for (int c = 0; c < SI_ALIEN_COLS && si_bullet_active; c++)
            for (int r = 0; r < SI_ALIEN_ROWS; r++) {
                if (!si_aliens[c][r]) continue;
                int ax = SI_ALIEN_OX + si_alien_ox + c * (SI_ALIEN_W + 2);
                int ay = SI_ALIEN_OY + si_alien_oy + r * (SI_ALIEN_H + 2);
                if (bx >= ax && bx < ax + SI_ALIEN_W && by >= ay && by < ay + SI_ALIEN_H) {
                    si_aliens[c][r] = 0;
                    si_score += 10;
                    si_bullet_active = 0;
                    break;
                }
            }
    }
    si_alien_tick++;
    if (si_alien_tick >= 25) {
        si_alien_tick = 0;
        int left_ok = 1, right_ok = 1;
        for (int r = 0; r < SI_ALIEN_ROWS; r++)
            for (int c = 0; c < SI_ALIEN_COLS; c++) {
                if (!si_aliens[c][r]) continue;
                int nx = SI_ALIEN_OX + si_alien_ox + c * (SI_ALIEN_W + 2);
                if (nx + si_alien_dx <= 0) left_ok = 0;
                if (nx + SI_ALIEN_W + si_alien_dx >= VIDEO_WIDTH) right_ok = 0;
            }
        if ((si_alien_dx < 0 && !left_ok) || (si_alien_dx > 0 && !right_ok)) {
            si_alien_dx = -si_alien_dx;
            si_alien_oy += 12;
            for (int c = 0; c < SI_ALIEN_COLS; c++)
                for (int r = SI_ALIEN_ROWS - 1; r >= 0; r--)
                    if (si_aliens[c][r]) {
                        int cy = SI_ALIEN_OY + si_alien_oy + r * (SI_ALIEN_H + 2);
                        if (cy + SI_ALIEN_H >= VIDEO_HEIGHT - SI_PLAYER_H - 5) si_quit = 1;
                        goto done_check;
                    }
done_check:;
        } else {
            si_alien_ox += si_alien_dx;
        }
    }
    for (int c = 0; c < SI_ALIEN_COLS; c++)
        for (int r = 0; r < SI_ALIEN_ROWS; r++)
            if (si_aliens[c][r]) return;
    si_win = 1;
    si_quit = 1;
}

static void space_invaders_draw(void) {
    clear_graphics_screen();
    for (int c = 0; c < SI_ALIEN_COLS; c++)
        for (int r = 0; r < SI_ALIEN_ROWS; r++) {
            if (!si_aliens[c][r]) continue;
            int gx = SI_ALIEN_OX + si_alien_ox + c * (SI_ALIEN_W + 2);
            int gy = SI_ALIEN_OY + si_alien_oy + r * (SI_ALIEN_H + 2);
            draw_rectangle(gx, gy, SI_ALIEN_W, SI_ALIEN_H, 0x00FF00);
        }
    draw_rectangle(si_px, VIDEO_HEIGHT - SI_PLAYER_H, SI_PLAYER_W, SI_PLAYER_H, 0xFFFFFF);
    if (si_bullet_active)
        draw_rectangle(si_bullet_x, si_bullet_y, SI_BULLET_W, SI_BULLET_H, 0xFFFF00);
    draw_text(5, 5, "Space Invaders", 0xFFFFFF);
    draw_text(5, 15, "Score:", 0xAAAAAA);
    draw_number(45, 15, (uint32_t)si_score, 0xFFFFFF);
    draw_text(180, 5, "L/R Space Esc", 0x666666);
}

void space_invaders_run(void) {
    while (!si_quit) {
        uint8_t sc;
        while ((sc = keyboard_get_scancode()) != 0) {
            if (sc & 0x80) continue;
            if (sc == 0x01) { si_quit = 1; break; }
            if (sc == 0x4B && si_px > 5) si_px -= 8;
            if (sc == 0x4D && si_px + SI_PLAYER_W < VIDEO_WIDTH - 5) si_px += 8;
            if (sc == 0x39) si_fire();
        }
        space_invaders_tick();
        game_history_tick();
        space_invaders_draw();
        for (volatile int d = 0; d < 15000; d++) ;
    }
    space_invaders_draw();
    if (si_win) draw_text(100, 90, "You Win!", 0x00FF00);
    else draw_text(95, 90, "Game Over", 0xFF0000);
    draw_text(80, 105, "Score:", 0xAAAAAA);
    draw_number(130, 105, (uint32_t)si_score, 0xFFFFFF);
    draw_text(70, 125, "Press any key", 0xAAAAAA);
    while (!keyboard_has_key()) { for (volatile int d = 0; d < 5000; d++) ; }
    while (keyboard_get_scancode() != 0) ;
}

void space_invaders_cleanup(void) {
    (void)0;
}

/* Racing: top-down, 3 lanes, car avoid obstacles. */
#define RACE_LANES      3
#define RACE_LANE_W     (VIDEO_WIDTH / RACE_LANES)
#define RACE_CAR_W      24
#define RACE_CAR_H      35
#define RACE_OBST_W     30
#define RACE_OBST_H     25
#define RACE_ROAD_Y     30
#define RACE_SCROLL     3
#define RACE_OBST_MAX   4

static int race_lane;
static int race_obst_lane[RACE_OBST_MAX];
static int race_obst_y[RACE_OBST_MAX];
static int race_obst_n;
static int race_score;
static int race_quit;
static int race_tick;

static int race_car_center_x(void) {
    return race_lane * RACE_LANE_W + RACE_LANE_W / 2;
}

void racing_init(void) {
    race_lane = 1;
    race_obst_n = 0;
    race_score = 0;
    race_quit = 0;
    race_tick = 0;
}

static void racing_spawn_obstacle(void) {
    if (race_obst_n >= RACE_OBST_MAX) return;
    race_obst_lane[race_obst_n] = race_tick % RACE_LANES;
    race_obst_y[race_obst_n] = -RACE_OBST_H;
    race_obst_n++;
}

static void racing_tick(void) {
    race_tick++;
    if (race_tick % 20 == 0) racing_spawn_obstacle();
    for (int i = 0; i < race_obst_n; i++) {
        race_obst_y[i] += RACE_SCROLL;
        if (race_obst_y[i] > VIDEO_HEIGHT) {
            race_score += 10;
            race_obst_n--;
            for (int j = i; j < race_obst_n; j++) {
                race_obst_lane[j] = race_obst_lane[j + 1];
                race_obst_y[j] = race_obst_y[j + 1];
            }
            i--;
        }
    }
    int car_cx = race_car_center_x();
    int car_y = VIDEO_HEIGHT - RACE_CAR_H - 15;
    for (int i = 0; i < race_obst_n; i++) {
        int ox = race_obst_lane[i] * RACE_LANE_W + RACE_LANE_W / 2 - RACE_OBST_W / 2;
        int oy = race_obst_y[i];
        if (oy + RACE_OBST_H < car_y || oy > car_y + RACE_CAR_H) continue;
        if (car_cx + RACE_CAR_W / 2 > ox && car_cx - RACE_CAR_W / 2 < ox + RACE_OBST_W)
            race_quit = 1;
    }
}

static void racing_draw(void) {
    clear_graphics_screen();
    for (int l = 0; l <= RACE_LANES; l++) {
        int x = l * RACE_LANE_W;
        for (int y = RACE_ROAD_Y; y < VIDEO_HEIGHT; y += 8)
            draw_rectangle(x, y, 4, 4, 0x444444);
    }
    for (int i = 0; i < race_obst_n; i++) {
        int ox = race_obst_lane[i] * RACE_LANE_W + RACE_LANE_W / 2 - RACE_OBST_W / 2;
        draw_rectangle(ox, race_obst_y[i], RACE_OBST_W, RACE_OBST_H, 0xFF0000);
    }
    int cx = race_car_center_x() - RACE_CAR_W / 2;
    draw_rectangle(cx, VIDEO_HEIGHT - RACE_CAR_H - 15, RACE_CAR_W, RACE_CAR_H, 0x0000FF);
    draw_text(5, 5, "Racing", 0xFFFFFF);
    draw_text(5, 15, "Score:", 0xAAAAAA);
    draw_number(45, 15, (uint32_t)race_score, 0xFFFFFF);
    draw_text(200, 5, "L/R dodge Esc quit", 0x666666);
}

void racing_run(void) {
    while (!race_quit) {
        uint8_t sc;
        while ((sc = keyboard_get_scancode()) != 0) {
            if (sc & 0x80) continue;
            if (sc == 0x01) { race_quit = 1; break; }
            if (sc == 0x4B && race_lane > 0) race_lane--;
            if (sc == 0x4D && race_lane < RACE_LANES - 1) race_lane++;
        }
        racing_tick();
        game_history_tick();
        racing_draw();
        for (volatile int d = 0; d < 35000; d++) ;
    }
    racing_draw();
    draw_text(95, 85, "Game Over", 0xFF0000);
    draw_text(80, 100, "Score:", 0xAAAAAA);
    draw_number(130, 100, (uint32_t)race_score, 0xFFFFFF);
    draw_text(70, 120, "Press any key", 0xAAAAAA);
    while (!keyboard_has_key()) { for (volatile int d = 0; d < 5000; d++) ; }
    while (keyboard_get_scancode() != 0) ;
}

void racing_cleanup(void) {
    (void)0;
}

// Stop current game
void stop_game(void) {
    if (game_running && current_game >= 0) {
        game_info_t* game = &games[current_game];
        if (game->cleanup_func) {
            game->cleanup_func();
        }
        game_running = 0;
        current_game = -1;
        kprint("Game stopped\n");
    }
}

// Get current game info
game_info_t* get_current_game(void) {
    if (current_game >= 0 && current_game < game_count) {
        return &games[current_game];
    }
    return NULL;
}

// Check if game is running
int is_game_running(void) {
    return game_running;
}
