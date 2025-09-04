#include "game.h"
#include "graphics.h"
#include "kernel.h"
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

// Launch game
void launch_game(int game_index) {
    if (game_index < 0 || game_index >= game_count) {
        kprint("Invalid game index!\n");
        return;
    }
    
    current_game = game_index;
    game_info_t* game = &games[game_index];
    
    kprintf("Launching %s...\n", game->name);
    kprintf("Description: %s\n", game->description);
    
    // Initialize game
    if (game->init_func) {
        game->init_func();
    }
    
    game_running = 1;
    
    // Run game
    if (game->run_func) {
        game->run_func();
    }
    
    // Cleanup game
    if (game->cleanup_func) {
        game->cleanup_func();
    }
    
    game_running = 0;
    current_game = -1;
    
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

// Snake game implementation
void snake_init(void) {
    kprint("Initializing Snake game...\n");
    // Initialize snake game state
}

void snake_run(void) {
    kprint("Running Snake game...\n");
    kprint("Use arrow keys to control snake\n");
    kprint("Eat food to grow longer\n");
    kprint("Don't hit walls or yourself!\n");
    
    // Simple snake game loop
    for (int i = 0; i < 100; i++) {
        // Game logic would go here
        for (volatile int j = 0; j < 10000; j++);
    }
}

void snake_cleanup(void) {
    kprint("Cleaning up Snake game...\n");
}

// Pong game implementation
void pong_init(void) {
    kprint("Initializing Pong game...\n");
    // Initialize pong game state
}

void pong_run(void) {
    kprint("Running Pong game...\n");
    kprint("Use up/down keys to move paddle\n");
    kprint("Keep the ball in play!\n");
    
    // Simple pong game loop
    for (int i = 0; i < 100; i++) {
        // Game logic would go here
        for (volatile int j = 0; j < 10000; j++);
    }
}

void pong_cleanup(void) {
    kprint("Cleaning up Pong game...\n");
}

// Tetris game implementation
void tetris_init(void) {
    kprint("Initializing Tetris game...\n");
    // Initialize tetris game state
}

void tetris_run(void) {
    kprint("Running Tetris game...\n");
    kprint("Use arrow keys to move/rotate blocks\n");
    kprint("Complete lines to score points!\n");
    
    // Simple tetris game loop
    for (int i = 0; i < 100; i++) {
        // Game logic would go here
        for (volatile int j = 0; j < 10000; j++);
    }
}

void tetris_cleanup(void) {
    kprint("Cleaning up Tetris game...\n");
}

// Space Invaders game implementation
void space_invaders_init(void) {
    kprint("Initializing Space Invaders game...\n");
    // Initialize space invaders game state
}

void space_invaders_run(void) {
    kprint("Running Space Invaders game...\n");
    kprint("Use left/right to move, space to shoot\n");
    kprint("Destroy all aliens to win!\n");
    
    // Simple space invaders game loop
    for (int i = 0; i < 100; i++) {
        // Game logic would go here
        for (volatile int j = 0; j < 10000; j++);
    }
}

void space_invaders_cleanup(void) {
    kprint("Cleaning up Space Invaders game...\n");
}

// Racing game implementation
void racing_init(void) {
    kprint("Initializing Racing game...\n");
    // Initialize racing game state
}

void racing_run(void) {
    kprint("Running Racing game...\n");
    kprint("Use left/right to steer\n");
    kprint("Avoid obstacles and stay on track!\n");
    
    // Simple racing game loop
    for (int i = 0; i < 100; i++) {
        // Game logic would go here
        for (volatile int j = 0; j < 10000; j++);
    }
}

void racing_cleanup(void) {
    kprint("Cleaning up Racing game...\n");
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
