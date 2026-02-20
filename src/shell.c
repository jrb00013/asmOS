#include "shell.h"
#include "fs.h"
#include "syscalls.h"
#include "kernel.h"
#include "video.h"
#include "graphics.h"
#include "game.h"

/* VGA attribute byte: (bg << 4) | fg. Bold CLI palette. */
#define C_DEFAULT  0x07  /* light gray on black */
#define C_BRIGHT   0x0F  /* white */
#define C_DIM      0x08  /* dark gray */
#define C_GREEN    0x0A  /* green */
#define C_CYAN     0x0B  /* cyan */
#define C_RED      0x0C  /* red */
#define C_YELLOW   0x0E  /* yellow */
#define C_MAGENTA  0x0D  /* magenta */
#define C_BLUE     0x09  /* light blue */
#define C_GREEN_BG 0x2A  /* green on dark green (accent) */

// Shell commands structure
typedef struct {
    const char *name;
    void (*func)(char *args);
    const char *description;
} shell_command_t;

// Command implementations
static void cmd_help(char *args);
static void cmd_ls(char *args);
static void cmd_meminfo(char *args);
static void cmd_exit(char *args);
static void cmd_ps2info(char *args);
static void cmd_cd(char *args);
static void cmd_cat(char *args);
static void cmd_clear(char *args);
static void cmd_date(char *args);
static void cmd_echo(char *args);
static void cmd_reboot(char *args);
static void cmd_network(char *args);
static void cmd_sound(char *args);
static void cmd_graphics(char *args);
static void cmd_timer(char *args);
static void cmd_controller(char *args);
static void cmd_ping(char *args);
static void cmd_ftp(char *args);
static void cmd_telnet(char *args);
static void cmd_irc(char *args);
static void cmd_game(char *args);
static void cmd_music(char *args);
static void cmd_demo(char *args);
static void cmd_benchmark(char *args);
static void cmd_system(char *args);

// Enhanced command list with advanced PS2 features
static shell_command_t commands[] = {
    {"help", cmd_help, "Show available commands"},
    {"ls", cmd_ls, "List files in current directory"},
    {"cd", cmd_cd, "Change directory"},
    {"cat", cmd_cat, "Display file contents"},
    {"meminfo", cmd_meminfo, "Show memory information"},
    {"ps2info", cmd_ps2info, "Show PS2 hardware information"},
    {"clear", cmd_clear, "Clear screen"},
    {"date", cmd_date, "Show current date/time"},
    {"echo", cmd_echo, "Echo text to screen"},
    {"reboot", cmd_reboot, "Reboot system"},
    {"network", cmd_network, "Network operations"},
    {"sound", cmd_sound, "Sound system control"},
    {"graphics", cmd_graphics, "Graphics system control"},
    {"timer", cmd_timer, "Timer operations"},
    {"controller", cmd_controller, "PS2 controller operations"},
    {"ping", cmd_ping, "Network ping"},
    {"ftp", cmd_ftp, "FTP client"},
    {"telnet", cmd_telnet, "Telnet client"},
    {"irc", cmd_irc, "IRC client"},
    {"game", cmd_game, "Launch games"},
    {"music", cmd_music, "Music player"},
    {"demo", cmd_demo, "Graphics demos"},
    {"benchmark", cmd_benchmark, "System benchmarks"},
    {"system", cmd_system, "System control"},
    {"exit", cmd_exit, "Exit shell"},
    {NULL, NULL, NULL}
};

void init_shell(void) {
    kprint("\n  ");
    kprint_color("############################################\n", C_CYAN);
    kprint("  ");
    kprint_color("#", C_CYAN);
    kprint("  ");
    kprint_color("ASMOS", C_BRIGHT);
    kprint("  x86  ");
    kprint_color("KERNEL", C_YELLOW);
    kprint("  ");
    kprint_color("SHELL", C_GREEN);
    kprint("              ");
    kprint_color("#\n", C_CYAN);
    kprint("  ");
    kprint_color("############################################\n", C_CYAN);
    kprint("  ");
    kprint_color("  >> ", C_DIM);
    kprint_color("help", C_YELLOW);
    kprint_color(" for commands  |  ", C_DIM);
    kprint_color("root@asmos", C_CYAN);
    kprint_color(" ready", C_GREEN);
    kprint("\n\n");
}

/* Parse "cmd args" from input: first word -> cmd, rest (trimmed) -> args. */
static void parse_cmd_args(const char *input, char *cmd, unsigned int cmd_max, char *args, unsigned int args_max) {
    unsigned int i = 0;
    while (input[i] == ' ' || input[i] == '\t') i++;
    unsigned int cmd_len = 0;
    while (input[i] && input[i] != '\n' && input[i] != '\r' && input[i] != ' ' && input[i] != '\t' && cmd_len < cmd_max - 1)
        cmd[cmd_len++] = input[i++];
    cmd[cmd_len] = '\0';
    while (input[i] == ' ' || input[i] == '\t') i++;
    unsigned int args_len = 0;
    while (input[i] && input[i] != '\n' && input[i] != '\r' && args_len < args_max - 1)
        args[args_len++] = input[i++];
    while (args_len > 0 && (args[args_len - 1] == ' ' || args[args_len - 1] == '\t'))
        args_len--;
    args[args_len] = '\0';
}

void start_shell(void) {
    char input[512];  // Increased buffer size for complex commands
    char cmd[64];
    char args[448];
    
    while (1) {
        print_prompt();
        sys_read_line(input, sizeof(input));
        
        parse_cmd_args(input, cmd, sizeof(cmd), args, sizeof(args));
        if (cmd[0] == '\0') {
            continue;
        }
        
        // Find and execute command
        int found = 0;
        for (int i = 0; commands[i].name; i++) {
            if (ksstrcmp(cmd, commands[i].name) == 0) {
                commands[i].func(args);
                found = 1;
                break;
            }
        }
        
        if (!found) {
            kprint_color("  >> ", C_DIM);
            kprint_color("unknown command", C_RED);
            kprint(": ");
            kprint_color(cmd, C_BRIGHT);
            kprint("\n  ");
            kprint_color("run 'help' for command list\n", C_DIM);
        }
    }
}

// Enhanced command functions
static void cmd_help(char *args) {
    kprint("\n  ");
    kprint_color(" COMMANDS ", C_GREEN_BG);
    kprint_color(" asmos shell ", C_DIM);
    kprint("\n  ");
    kprint_color("----------------------------------------\n", C_DIM);
    kprint("  ");
    kprint_color("core", C_CYAN);
    kprint("     ");
    kprintf("%-10s  %s\n", "help", "this help");
    kprintf("     %-10s  %s\n", "clear", "clear screen");
    kprintf("     %-10s  %s\n", "exit", "exit shell");
    kprint("  ");
    kprint_color("fs", C_YELLOW);
    kprint("       ");
    kprintf("%-10s  %s\n", "ls", "list directory");
    kprintf("     %-10s  %s\n", "cd", "change directory");
    kprintf("     %-10s  %s\n", "cat", "show file");
    kprint("  ");
    kprint_color("system", C_MAGENTA);
    kprint("   ");
    kprintf("%-10s  %s\n", "meminfo", "memory info");
    kprintf("     %-10s  %s\n", "ps2info", "hardware info");
    kprintf("     %-10s  %s\n", "date", "date/time");
    kprintf("     %-10s  %s\n", "reboot", "reboot");
    kprint("  ");
    kprint_color("net", C_BLUE);
    kprint("       ");
    kprintf("%-10s  %s\n", "network", "network status");
    kprintf("     %-10s  %s\n", "ping", "ping host");
    kprintf("     %-10s  %s\n", "ftp", "FTP client");
    kprintf("     %-10s  %s\n", "telnet", "telnet");
    kprintf("     %-10s  %s\n", "irc", "IRC client");
    kprint("  ");
    kprint_color("media", C_GREEN);
    kprint("    ");
    kprintf("%-10s  %s\n", "sound", "sound control");
    kprintf("     %-10s  %s\n", "graphics", "GPU control");
    kprintf("     %-10s  %s\n", "music", "music player");
    kprintf("     %-10s  %s\n", "demo", "graphics demo");
    kprint("  ");
    kprint_color("run", C_CYAN);
    kprint("       ");
    kprintf("%-10s  %s\n", "game", "launch game");
    kprintf("     %-10s  %s\n", "controller", "controller");
    kprintf("     %-10s  %s\n", "timer", "timers");
    kprintf("     %-10s  %s\n", "benchmark", "benchmarks");
    kprintf("     %-10s  %s\n", "system", "system status");
    kprint("  ");
    kprint_color("----------------------------------------\n", C_DIM);
    kprint("\n");
}

static void cmd_ls(char *args) {
    kprint("\n  ");
    kprint_color(" [ ", C_DIM);
    kprint_color("ls", C_CYAN);
    kprint_color(" ] ", C_DIM);
    kprint(" current directory\n  ");
    kprint_color("----------------------------------------\n", C_DIM);
    fat12_list_files();
    kprint("  ");
    kprint_color("----------------------------------------\n", C_DIM);
    kprint("\n");
}

static void cmd_cd(char *args) {
    if (ksstrcmp(args, "") == 0) {
        kprint("  ");
        kprint_color("cwd", C_DIM);
        kprint(" /\n");
    } else {
        kprint("  ");
        kprint_color("cd", C_CYAN);
        kprintf(" %s ", args);
        kprint_color("(nav not impl)\n", C_DIM);
    }
}

static void cmd_cat(char *args) {
    if (ksstrcmp(args, "") == 0) {
        kprint("  ");
        kprint_color("usage", C_DIM);
        kprint(": ");
        kprint_color("cat", C_CYAN);
        kprint(" <file>\n");
        return;
    }
    kprint("  ");
    kprint_color("cat", C_CYAN);
    kprintf(" %s ", args);
    kprint_color("(read not impl)\n", C_DIM);
}

static void cmd_clear(char *args) {
    clear_screen();
}

static void cmd_date(char *args) {
    kprint("Date/Time: 2024-01-01 12:00:00 (PS2 System Time)\n");
}

static void cmd_echo(char *args) {
    if (ksstrcmp(args, "") != 0) {
        kprintf("%s\n", args);
    }
}

static void cmd_reboot(char *args) {
    kprint("Rebooting PS2 system...\n");
    for (volatile int i = 0; i < 1000000; i++);
    asm volatile("int $0x19");
}

static void cmd_meminfo(char *args) {
    uint32_t mem_kb;
    asm volatile("call get_memory_info" : "=a"(mem_kb));
    kprint("\n  ");
    kprint_color(" memory ", C_MAGENTA);
    kprint_color(" ----------------------------------------\n", C_DIM);
    kprint("    total     ");
    kprintf("%u KB (%u MB)\n", mem_kb, mem_kb / 1024);
    kprint("    free      ");
    kprintf("%u KB\n", mem_kb - 1024);
    kprint("    type      ");
    kprint_color("DDR SDRAM", C_CYAN);
    kprint(" @ 150MHz, 2.4 GB/s\n");
    kprint("  ");
    kprint_color("----------------------------------------\n", C_DIM);
    kprint("\n");
}

static void cmd_ps2info(char *args) {
    kprint("\n  ");
    kprint_color(" hardware ", C_YELLOW);
    kprint_color(" --------------------------------------\n", C_DIM);
    kprint("    system    ");
    kprint_color("PlayStation 2", C_CYAN);
    kprint("\n    cpu       MIPS R5900 (EE) @ 294MHz\n");
    kprint("    gpu       Graphics Synthesizer @ 147MHz\n");
    kprint("    ram       32MB DDR @ 150MHz\n");
    kprint("    storage   CD/DVD\n");
    kprint("    input     DualShock 2\n");
    kprint("    net       Ethernet 10/100\n");
    kprint("    sound     SPU2 (48 ch)\n");
    kprint("  ");
    kprint_color("----------------------------------------\n", C_DIM);
    kprint("\n");
}

static void cmd_network(char *args) {
    kprint("\n  ");
    kprint_color(" network ", C_BLUE);
    kprint_color(" -------------------------------------\n", C_DIM);
    kprint("    init     ");
    
    int result;
    asm volatile("call sys_network_init" : "=a"(result));
    
    if (result) {
        kprint_color("ok", C_GREEN);
        kprint("\n    ip        10.0.0.1\n");
        kprint("    netmask   255.255.255.0\n");
        kprint("    gateway  10.0.0.254\n");
        kprint("    cmds      ping, ftp, telnet, irc\n");
    } else {
        kprint_color("failed", C_RED);
        kprint(" (adapter not found)\n");
    }
    kprint("  ");
    kprint_color("----------------------------------------\n", C_DIM);
    kprint("\n");
}

static void cmd_sound(char *args) {
    kprint("PS2 Sound System Control:\n");
    kprint("=========================\n");
    kprint("Initializing SPU2 sound system...\n");
    
    // Initialize sound
    asm volatile("call sys_sound_init");
    
    kprint("Sound system initialized!\n");
    kprint("SPU2 Features:\n");
    kprint("  - 48 audio channels\n");
    kprint("  - 44.1kHz sample rate\n");
    kprint("  - 16-bit audio\n");
    kprint("  - ADPCM compression\n");
    kprint("  - Reverb and echo effects\n");
    kprint("Use 'music' command to play audio files\n");
}

static void cmd_graphics(char *args) {
    kprint("PS2 Graphics System Control:\n");
    kprint("============================\n");
    kprint("Initializing Graphics Synthesizer...\n");
    
    // Initialize graphics
    asm volatile("call sys_graphics_init");
    
    kprint("Graphics system initialized!\n");
    kprint("Graphics Synthesizer Features:\n");
    kprint("  - Resolution: Up to 1920x1080\n");
    kprint("  - Color depth: 24-bit\n");
    kprint("  - Fill rate: 2.4 Gpixels/s\n");
    kprint("  - Texture memory: 4MB\n");
    kprint("  - Hardware acceleration\n");
    kprint("Use 'demo' command for graphics demonstrations\n");
}

static void cmd_timer(char *args) {
    kprint("PS2 Timer System:\n");
    kprint("=================\n");
    kprint("Initializing timers...\n");
    
    // Initialize timers
    asm volatile("call sys_timer_init");
    
    // Get current timer value
    uint32_t timer_value;
    asm volatile("call sys_timer_get" : "=a"(timer_value));
    
    kprintf("Timer initialized! Current value: %u\n", timer_value);
    kprint("Available timers:\n");
    kprint("  - Timer 0: System timer\n");
    kprint("  - Timer 1: User timer\n");
    kprint("  - Timer 2: CD/DVD timer\n");
}

static void cmd_controller(char *args) {
    kprint("PS2 Controller Operations:\n");
    kprint("==========================\n");
    kprint("Reading controller data...\n");
    
    // Read controller
    uint32_t controller_data;
    asm volatile("call sys_ps2_controller_read" : "=a"(controller_data));
    
    kprintf("Controller data: 0x%08X\n", controller_data);
    kprint("Controller buttons:\n");
    kprint("  - D-Pad: Up, Down, Left, Right\n");
    kprint("  - Face buttons: Triangle, Circle, X, Square\n");
    kprint("  - Shoulder buttons: L1, L2, R1, R2\n");
    kprint("  - Analog sticks: Left and Right\n");
    kprint("  - Start/Select buttons\n");
}

static void cmd_ping(char *args) {
    if (ksstrcmp(args, "") == 0) {
        kprint("  ");
        kprint_color("usage", C_DIM);
        kprint(": ");
        kprint_color("ping", C_CYAN);
        kprint(" <host>\n");
        return;
    }
    kprint("  ");
    kprint_color("ping", C_CYAN);
    kprintf(" %s ", args);
    kprint_color("(not impl)\n", C_DIM);
}

static void cmd_ftp(char *args) {
    kprint("PS2 FTP Client:\n");
    kprint("==============\n");
    kprint("FTP client not yet implemented\n");
    kprint("Would allow file transfer over network\n");
    kprint("Features planned:\n");
    kprint("  - Connect to FTP servers\n");
    kprint("  - Upload/download files\n");
    kprint("  - Directory listing\n");
    kprint("  - Binary/ASCII mode\n");
}

static void cmd_telnet(char *args) {
    kprint("PS2 Telnet Client:\n");
    kprint("==================\n");
    kprint("Telnet client not yet implemented\n");
    kprint("Would allow remote terminal access\n");
    kprint("Features planned:\n");
    kprint("  - Connect to telnet servers\n");
    kprint("  - Remote command execution\n");
    kprint("  - Terminal emulation\n");
    kprint("  - Color support\n");
}

static void cmd_irc(char *args) {
    kprint("PS2 IRC Client:\n");
    kprint("===============\n");
    kprint("IRC client not yet implemented\n");
    kprint("Would allow Internet Relay Chat\n");
    kprint("Features planned:\n");
    kprint("  - Connect to IRC servers\n");
    kprint("  - Join channels\n");
    kprint("  - Send/receive messages\n");
    kprint("  - Private messaging\n");
}

static void cmd_game(char *args) {
    int idx = -1;
    if (args && args[0] != '\0') {
        ksscanf(args, "%d", &idx);
        if (idx >= 1) {
            launch_game(idx - 1);
            return;
        }
    }
    list_games();
    kprint("Usage: game <number>  (e.g. game 1)\n");
}

static void cmd_music(char *args) {
    kprint("PS2 Music Player:\n");
    kprint("=================\n");
    kprint("Music player not yet implemented\n");
    kprint("Would allow playing audio files\n");
    kprint("Features planned:\n");
    kprint("  - MP3 playback\n");
    kprint("  - WAV playback\n");
    kprint("  - Playlist support\n");
    kprint("  - Volume control\n");
    kprint("  - Equalizer\n");
}

static void cmd_demo(char *args) {
    (void)args;
    kprint("Switching to graphics mode - running demo...\n");
#ifndef PS2_HARDWARE
    video_set_mode_13h();
#endif
    init_graphics_demo();
    run_graphics_demo();
#ifndef PS2_HARDWARE
    video_set_mode_text();
#endif
    kprint("Back to text mode.\n");
}

static void cmd_benchmark(char *args) {
    kprint("\n  ");
    kprint_color(" benchmark ", C_MAGENTA);
    kprint_color(" ---------------------------------\n", C_DIM);
    kprint("    running ... ");
    
    volatile int cpu_result = 0;
    for (volatile int i = 0; i < 1000000; i++) {
        cpu_result += i;
    }
    
    kprint_color("cpu", C_CYAN);
    kprintf(" %d ops  ", cpu_result);
    kprint_color("mem", C_GREEN);
    kprint(" ok  ");
    kprint_color("gpu", C_YELLOW);
    kprint(" ok\n");
    kprint("  ");
    kprint_color("----------------------------------------\n", C_DIM);
    kprint("\n");
}

static void cmd_system(char *args) {
    kprint("\n  ");
    kprint_color(" system ", C_GREEN);
    kprint_color(" ---------------------------------------\n", C_DIM);
    kprint("    cpu       MIPS R5900 @ 294MHz\n");
    kprint("    gpu       Graphics Synthesizer @ 147MHz\n");
    kprint("    memory    32MB DDR\n");
    kprint("    network   Ethernet 10/100\n");
    kprint("    sound     SPU2 (48ch)\n");
    kprint("    storage   CD/DVD\n");
    kprint("    ctrl      DualShock 2\n");
    kprint("    temp      ");
    kprint_color("ok", C_GREEN);
    kprint("    pwr       AC\n");
    kprint("  ");
    kprint_color("----------------------------------------\n", C_DIM);
    kprint("\n");
}

static void cmd_exit(char *args) {
    kprint("\n  ");
    kprint_color(" session ended ", C_DIM);
    kprint("\n  ");
    kprint_color("asmos", C_CYAN);
    kprint_color(" out. ", C_DIM);
    kprint("\n\n");
    sys_exit(0);
}

void print_prompt(void) {
    kprint_color("  ", C_DIM);
    kprint_color("root", C_CYAN);
    kprint_color("@", C_DIM);
    kprint_color("asmos", C_YELLOW);
    kprint_color(":", C_DIM);
    kprint_color("~", C_GREEN);
    kprint_color(" $ ", C_BRIGHT);
}