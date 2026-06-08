#include "shell.h"
#include "fs.h"
#include "syscalls.h"
#ifdef PLATFORM_PS2
/* PS2 syscall implementations in platform/ps2/io_syscalls.c */
#endif
#include "kernel.h"
#include "video.h"
#include "graphics.h"
#include "game.h"
#include "game_history.h"
#include "music.h"
#include "controller_remap.h"
#include "save_manager.h"
#include "dashboard.h"
#include "party.h"
#include "streaming.h"
#include "transport.h"
#include "bluetooth.h"
#include "hw_status.h"
#include "memory_card.h"
#include "storage.h"
#include "led_control.h"
#include "pause_engine.h"
#include "platform.h"
#include "net.h"
#include "net_clients.h"
#include "subsys.h"
#include "quantum.h"

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
static void cmd_observe(char *args);
static void cmd_collapse(char *args);
static void cmd_entangle(char *args);
static void cmd_coherence(char *args);
static void cmd_superpose(char *args);
static void cmd_ftp(char *args);
static void cmd_telnet(char *args);
static void cmd_irc(char *args);
static void cmd_game(char *args);
static void cmd_games(char *args);
static void cmd_music(char *args);
static void cmd_demo(char *args);
static void cmd_benchmark(char *args);
static void cmd_system(char *args);
static void cmd_party(char *args);
static void cmd_stream(char *args);
static void cmd_bt(char *args);
static void cmd_ctrlmap(char *args);
static void cmd_ctrlprofile(char *args);
/* Johnny: Kernel & Hardware CLI */
static void cmd_sysinfo(char *args);
static void cmd_memstat(char *args);
static void cmd_ports(char *args);
static void cmd_iopstat(char *args);
static void cmd_temp(char *args);
static void cmd_mc(char *args);
static void cmd_led(char *args);
static void cmd_saves(char *args);
static void cmd_dashboard(char *args);

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
    {"observe", cmd_observe, "Observe subsystem into RAM"},
    {"collapse", cmd_collapse, "Collapse config profile"},
    {"superpose", cmd_superpose, "Superpose two profiles"},
    {"entangle", cmd_entangle, "Entangle controller ports"},
    {"coherence", cmd_coherence, "Show coherence status"},
    {"ftp", cmd_ftp, "FTP client"},
    {"telnet", cmd_telnet, "Telnet client"},
    {"irc", cmd_irc, "IRC client"},
    {"game", cmd_game, "Launch games"},
    {"games", cmd_games, "Game history (history|stats|last)"},
    {"music", cmd_music, "Music player"},
    {"demo", cmd_demo, "Graphics demos"},
    {"benchmark", cmd_benchmark, "System benchmarks"},
    {"system", cmd_system, "System control"},
    {"sysinfo", cmd_sysinfo, "Full system status (EE, RAM, IOP, ports, net, temp)"},
    {"memstat", cmd_memstat, "Memory usage"},
    {"ports", cmd_ports, "Controller/USB port status"},
    {"iopstat", cmd_iopstat, "IOP status"},
    {"temp", cmd_temp, "Temperature status"},
    {"mc", cmd_mc, "Memory card: list, mount, export, clone, repair"},
    {"led", cmd_led, "LED: set, pulse, rgb"},
    {"party", cmd_party, "Party: create/invite/list/join"},
    {"stream", cmd_stream, "Stream gameplay to PC"},
    {"bt", cmd_bt, "Bluetooth: scan/pair"},
    {"ctrlmap", cmd_ctrlmap, "Controller mapping"},
    {"ctrlprofile", cmd_ctrlprofile, "Controller profile"},
    {"saves", cmd_saves, "Save backup/restore/clone/versions"},
    {"dashboard", cmd_dashboard, "Visual dashboard (memory, LED, status)"},
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
        subsys_tick_all();
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
    kprintf("     %-10s  %s\n", "sysinfo", "full system status");
    kprintf("     %-10s  %s\n", "memstat", "memory usage");
    kprintf("     %-10s  %s\n", "ports", "controller/USB ports");
    kprintf("     %-10s  %s\n", "iopstat", "IOP status");
    kprintf("     %-10s  %s\n", "temp", "temperature");
    kprintf("     %-10s  %s\n", "date", "date/time");
    kprintf("     %-10s  %s\n", "reboot", "reboot");
    kprint("  ");
    kprint_color("mc/led", C_YELLOW);
    kprint("   ");
    kprintf("%-10s  %s\n", "mc", "memory card list/mount/export/clone/repair");
    kprintf("     %-10s  %s\n", "led", "LED set/pulse/rgb");
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
    (void)args;
    kprint("\n  ");
    kprint_color(" network ", C_BLUE);
    kprint_color(" -------------------------------------\n", C_DIM);
    kprint("    init     ");
    int result = plat_net_init();
    net_init();
    plat_net_info_t ni;
    plat_net_get_info(&ni);
    if (result == 0) {
        kprint_color("ok", C_GREEN);
        kprintf("\n    ip        %s\n", ni.ip_str);
        kprint("    netmask   255.255.255.0\n");
        kprint("    gateway   10.0.0.254\n");
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
    
#ifdef PLATFORM_PS2
    sys_sound_init();
#else
    asm volatile("call sys_sound_init");
#endif
    
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
    
#ifdef PLATFORM_PS2
    sys_graphics_init();
#else
    asm volatile("call sys_graphics_init");
#endif
    
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
    
#ifdef PLATFORM_PS2
    sys_timer_init();
    uint32_t timer_value = sys_timer_get();
#else
    asm volatile("call sys_timer_init");
    uint32_t timer_value;
    asm volatile("call sys_timer_get" : "=a"(timer_value));
#endif
    
    kprintf("Timer initialized! Current value: %u\n", timer_value);
    kprint("Available timers:\n");
    kprint("  - Timer 0: System timer\n");
    kprint("  - Timer 1: User timer\n");
    kprint("  - Timer 2: CD/DVD timer\n");
}

static void cmd_controller(char *args) {
    char sub[32], name[CTRL_PROFILE_NAME_LEN], name2[CTRL_PROFILE_NAME_LEN];
    sub[0] = name[0] = name2[0] = '\0';
    if (args && args[0] != '\0') {
        ksscanf(args, "%31s %23s %23s", sub, name, name2);
        if (ksstrcmp(sub, "profile") == 0) {
            if (ksstrcmp(name, "list") == 0) {
                controller_profile_list();
                return;
            }
            if (ksstrcmp(name, "save") == 0 && name2[0]) {
                if (controller_profile_save(name2) == 0)
                    kprintf("  Profile '%s' saved.\n", name2);
                else
                    kprint("  Failed to save profile.\n");
                return;
            }
            if (ksstrcmp(name, "load") == 0 && name2[0]) {
                if (controller_profile_load(name2) == 0)
                    kprintf("  Profile '%s' loaded.\n", name2);
                else
                    kprint("  Profile not found.\n");
                return;
            }
            if (ksstrcmp(name, "apply") == 0 && name2[0]) {
                controller_profile_apply(name2);
                return;
            }
        }
        if (ksstrcmp(sub, "sensitivity") == 0 && name[0]) {
            int val = 128;
            ksscanf(name, "%d", &val);
            controller_set_sensitivity(val);
            kprintf("  Sensitivity set to %d\n", val);
            return;
        }
        if (ksstrcmp(sub, "deadzone") == 0 && name[0]) {
            int val = 32;
            ksscanf(name, "%d", &val);
            controller_set_deadzone(val);
            kprintf("  Deadzone set to %d\n", val);
            return;
        }
        if (ksstrcmp(sub, "turbo") == 0) {
            if (ksstrcmp(name, "on") == 0) controller_set_turbo(0xFFFF);
            else if (ksstrcmp(name, "off") == 0) controller_set_turbo(0);
            else if (name[0]) {
                int hex = 0;
                ksscanf(name, "%x", &hex);
                controller_set_turbo(hex);
            }
            controller_remap_print_current();
            return;
        }
        if (ksstrcmp(sub, "current") == 0) {
            controller_remap_print_current();
            return;
        }
    }
    controller_remap_print_current();
    kprint("  controller profile list|save|load|apply <name>\n");
    kprint("  controller sensitivity|deadzone <0-255>  turbo on|off|<hex>\n");
}

static void cmd_observe(char *args) {
    if (!args || !args[0]) { kprint("  usage: observe <subsys>\n"); return; }
    quantum_observe(args);
}

static void cmd_collapse(char *args) {
    if (!args || !args[0]) { kprint("  usage: collapse <profile>\n"); return; }
    if (quantum_collapse(args) != 0) kprint_color("  collapse failed\n", C_RED);
}

static void cmd_superpose(char *args) {
    char a[24], b[24];
    a[0] = b[0] = '\0';
    if (args) ksscanf(args, "%23s %23s", a, b);
    if (!a[0] || !b[0]) { kprint("  usage: superpose <a> <b>\n"); return; }
    quantum_superpose(a, b, 128);
}

static void cmd_entangle(char *args) {
    int pa = 0, pb = 1;
    if (args) ksscanf(args, "%d %d", &pa, &pb);
    quantum_entangle_input(pa, pb);
}

static void cmd_coherence(char *args) {
    char buf[96];
    (void)args;
    quantum_status(buf, sizeof(buf));
    kprintf("  coherence: %s\n", buf);
    subsys_status_all(buf, sizeof(buf));
    kprintf("  subsys: %s\n", buf);
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
    uint32_t rtt;
    kprintf("  ping %s ... ", args);
    if (plat_net_ping(args, &rtt) == 0)
        kprintf("ok (%u ms)\n", (unsigned)rtt);
    else
        kprint_color("timeout\n", C_RED);
}

static void cmd_ftp(char *args) {
    char host[64];
    host[0] = '\0';
    if (args && args[0]) ksscanf(args, "%63s", host);
    if (!host[0]) {
        kprint("  usage: ftp <host> [path]\n");
        return;
    }
    ftp_client(host, args);
}

static void cmd_telnet(char *args) {
    char host[64];
    host[0] = '\0';
    if (args && args[0]) ksscanf(args, "%63s", host);
    if (!host[0]) {
        kprint("  usage: telnet <host>\n");
        return;
    }
    telnet_client(host, args);
}

static void cmd_irc(char *args) {
    char host[64], chan[32];
    host[0] = chan[0] = '\0';
    if (args && args[0]) ksscanf(args, "%63s %31s", host, chan);
    if (!host[0]) {
        kprint("  usage: irc <host> [channel]\n");
        return;
    }
    irc_client(host, chan[0] ? chan : "#asmos", "asmos");
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

static void cmd_games(char *args) {
    char sub[32];
    sub[0] = '\0';
    if (args && args[0] != '\0')
        ksscanf(args, "%31s", sub);
    if (ksstrcmp(sub, "history") == 0) {
        game_history_print_history();
        return;
    }
    if (ksstrcmp(sub, "stats") == 0) {
        game_history_print_stats();
        return;
    }
    if (ksstrcmp(sub, "last") == 0) {
        game_history_print_last();
        return;
    }
    kprint("  ");
    kprint_color("games", C_CYAN);
    kprint(" <history|stats|last>\n");
    kprint("    history  list all game sessions\n");
    kprint("    stats    total playtime, most played\n");
    kprint("    last     last played game\n");
}

static void cmd_music(char *args) {
    char sub[32], arg2[64], arg3[MUSIC_PATH_LEN];
    sub[0] = arg2[0] = arg3[0] = '\0';
    if (args && args[0] != '\0') {
        ksscanf(args, "%31s %63s %63s", sub, arg2, arg3);
        if (ksstrcmp(sub, "play") == 0) {
            music_play(arg2[0] ? arg2 : 0);
            return;
        }
        if (ksstrcmp(sub, "list") == 0) {
            music_list(arg2[0] ? arg2 : 0);
            return;
        }
        if (ksstrcmp(sub, "background") == 0) {
            if (ksstrcmp(arg2, "on") == 0) music_background(1);
            else if (ksstrcmp(arg2, "off") == 0) music_background(0);
            else music_print_status();
            return;
        }
        if (ksstrcmp(sub, "status") == 0) {
            music_print_status();
            return;
        }
        if (ksstrcmp(sub, "playlist") == 0) {
            if (ksstrcmp(arg2, "add") == 0) {
                if (arg3[0]) {
                    if (music_add_to_playlist(arg3) == 0)
                        kprintf("  Added to playlist: %s\n", arg3);
                    else
                        kprint("  Playlist full.\n");
                } else
                    kprint("  usage: music playlist add <file>\n");
                return;
            }
            if (ksstrcmp(arg2, "list") == 0) {
                music_playlist_list();
                return;
            }
            if (ksstrcmp(arg2, "clear") == 0) {
                music_playlist_clear();
                kprint("  Playlist cleared.\n");
                return;
            }
        }
    }
    kprint("  ");
    kprint_color("music", C_GREEN);
    kprint(" play|list|background|status|playlist <add|list|clear>\n");
    kprint("    play <file>       play file (MP3/WAV)\n");
    kprint("    list [path]       list audio files\n");
    kprint("    background on|off toggle background playback\n");
    kprint("    playlist add <f>  add file to playlist\n");
    kprint("    playlist list     list playlist\n");
    kprint("    playlist clear    clear playlist\n");
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

/* ---- Johnny: Hardware Status CLI ---- */
static void cmd_sysinfo(char *args) {
    (void)args;
    hw_sysinfo_t si;
    hw_status_get_sysinfo(&si);
    kprint("\n  ");
    kprint_color(" sysinfo ", C_MAGENTA);
    kprint_color(" ----------------------------------------\n", C_DIM);
    kprintf("    model    %s\n", si.model);
    kprintf("    ee       %u MHz  load %u%%\n", si.ee_mhz, si.ee_load_percent);
    kprintf("    ram      %u MB\n", si.ram_mb);
    kprintf("    mem      %u KB total  %u KB free  %u KB used\n",
            si.memstat.total_kb, si.memstat.free_kb, si.memstat.used_kb);
    kprint("    iop      ");
    kprint_color(si.iop.status, si.iop.running ? C_GREEN : C_RED);
    kprintf("  load %u%%\n", si.iop.load_percent);
    kprint("    network  ");
    kprint_color(si.network.info, si.network.linked ? C_GREEN : C_DIM);
    kprint("\n");
    kprint("    temp     ");
    kprint_color(si.temp.status, C_YELLOW);
    kprint("\n  ");
    kprint_color("----------------------------------------\n", C_DIM);
    kprint("\n");
}

static void cmd_memstat(char *args) {
    (void)args;
    hw_memstat_t m;
    hw_status_get_memstat(&m);
    kprint("\n  ");
    kprint_color(" memstat ", C_CYAN);
    kprint_color(" ---------------------------------------\n", C_DIM);
    kprintf("    total   %u KB  (%u MB)\n", m.total_kb, m.total_kb / 1024);
    kprintf("    free    %u KB\n", m.free_kb);
    kprintf("    used    %u KB\n", m.used_kb);
    kprintf("    kernel  %u KB\n", m.kernel_kb);
    kprint("  ");
    kprint_color("----------------------------------------\n", C_DIM);
    kprint("\n");
}

static void cmd_ports(char *args) {
    (void)args;
    hw_port_status_t p[HW_STATUS_MAX_PORTS];
    hw_status_get_ports(p, HW_STATUS_MAX_PORTS);
    kprint("\n  ");
    kprint_color(" ports ", C_YELLOW);
    kprint_color(" ------------------------------------------\n", C_DIM);
    for (int i = 0; i < HW_STATUS_MAX_PORTS; i++) {
        kprintf("    port %d  ", i + 1);
        kprint_color(p[i].present ? "present" : "empty", p[i].present ? C_GREEN : C_DIM);
        kprintf("  %s  devices %d\n", p[i].info, p[i].device_count);
    }
    kprint("  ");
    kprint_color("----------------------------------------\n", C_DIM);
    kprint("\n");
}

static void cmd_iopstat(char *args) {
    (void)args;
    hw_iop_status_t iop;
    hw_status_get_iop(&iop);
    kprint("\n  ");
    kprint_color(" iopstat ", C_GREEN);
    kprint_color(" -----------------------------------------\n", C_DIM);
    kprint("    status  ");
    kprint_color(iop.running ? "running" : "halted", iop.running ? C_GREEN : C_RED);
    kprintf("\n    load    %u%%\n", iop.load_percent);
    kprint("  ");
    kprint_color("----------------------------------------\n", C_DIM);
    kprint("\n");
}

static void cmd_temp(char *args) {
    (void)args;
    hw_temp_status_t t;
    hw_status_get_temp(&t);
    kprint("\n  ");
    kprint_color(" temp ", C_RED);
    kprint_color(" ---------------------------------------------\n", C_DIM);
    kprint("    sensor  ");
    kprint_color(t.detected ? "ok" : "n/a", t.detected ? C_GREEN : C_DIM);
    if (t.celsius >= 0) kprintf("  %d C", t.celsius);
    kprintf("\n    status  %s\n", t.status);
    kprint("  ");
    kprint_color("----------------------------------------\n", C_DIM);
    kprint("\n");
}

/* Parse next word from s into buf; return pointer past word. */
static const char *next_word(const char *s, char *buf, unsigned int buf_max) {
    while (*s == ' ' || *s == '\t') s++;
    unsigned int i = 0;
    while (*s && *s != ' ' && *s != '\t' && i < buf_max - 1) buf[i++] = *s++;
    buf[i] = '\0';
    return s;
}

static void cmd_mc(char *args) {
    char sub[32];
    const char *rest = next_word(args, sub, sizeof(sub));
    if (sub[0] == '\0') {
        kprint("  ");
        kprint_color("mc", C_CYAN);
        kprint(" list | mount <1|2> | export <file> [save] | clone <slot1> <slot2> | repair\n");
        return;
    }
    if (ksstrcmp(sub, "list") == 0) {
        mc_status_t st;
        mc_get_status(&st);
        kprint("\n  ");
        kprint_color(" mc list ", C_CYAN);
        kprint(" (slot ");
        kprintf("%d mounted) ", st.mounted + 1);
        kprint_color("--------------------------------\n", C_DIM);
        fat12_list_files();
        kprint("  ");
        kprint_color("----------------------------------------\n", C_DIM);
        kprint("\n");
        return;
    }
    if (ksstrcmp(sub, "mount") == 0) {
        char slot_str[16];
        next_word(rest, slot_str, sizeof(slot_str));
        int slot = 0;
        ksscanf(slot_str, "%d", &slot);
        if (slot < 1 || slot > 2) {
            kprint("  mc mount: use 1 or 2\n");
            return;
        }
        int r = mc_mount(slot - 1);
        if (r == 0)
            kprintf("  mc: mounted slot %d\n", slot);
        else
            kprint("  mc mount: failed\n");
        return;
    }
    if (ksstrcmp(sub, "export") == 0) {
        char fname[64], save[64];
        rest = next_word(rest, fname, sizeof(fname));
        next_word(rest, save, sizeof(save));
        if (fname[0] == '\0') {
            kprint("  mc export <filename> [save_name]\n");
            return;
        }
        mc_export(fname, save[0] ? save : NULL);
        return;
    }
    if (ksstrcmp(sub, "clone") == 0) {
        char a[16], b[16];
        rest = next_word(rest, a, sizeof(a));
        next_word(rest, b, sizeof(b));
        int s1 = 0, s2 = 0;
        ksscanf(a, "%d", &s1);
        ksscanf(b, "%d", &s2);
        if (s1 < 1 || s1 > 2 || s2 < 1 || s2 > 2) {
            kprint("  mc clone slot1 slot2  (1 or 2)\n");
            return;
        }
        int r = mc_clone(s1 - 1, s2 - 1);
        if (r == 0) kprint("  mc clone: ok\n");
        return;
    }
    if (ksstrcmp(sub, "repair") == 0) {
        int r = mc_repair(-1);
        if (r == 0) kprint("  mc repair: done\n");
        return;
    }
    kprint("  mc: unknown subcommand (list, mount, export, clone, repair)\n");
}

static void cmd_led(char *args) {
    char sub[32];
    const char *rest = next_word(args, sub, sizeof(sub));
    if (sub[0] == '\0') {
        kprint("  ");
        kprint_color("led", C_YELLOW);
        kprint(" set <red|green|blue|off> | pulse [ms] | rgb <r> <g> <b>\n");
        return;
    }
    if (ksstrcmp(sub, "set") == 0) {
        char color[16];
        next_word(rest, color, sizeof(color));
        if (color[0] == '\0') {
            kprint("  led set red|green|blue|off\n");
            return;
        }
        int r = led_set(color);
        if (r == 0) kprintf("  led set %s\n", color);
        return;
    }
    if (ksstrcmp(sub, "pulse") == 0) {
        char ms_str[16];
        next_word(rest, ms_str, sizeof(ms_str));
        uint32_t ms = 0;
        ksscanf(ms_str, "%u", &ms);
        int r = led_pulse(ms);
        if (r == 0) kprint("  led pulse\n");
        return;
    }
    if (ksstrcmp(sub, "rgb") == 0) {
        char r_str[8], g_str[8], b_str[8];
        rest = next_word(rest, r_str, sizeof(r_str));
        rest = next_word(rest, g_str, sizeof(g_str));
        next_word(rest, b_str, sizeof(b_str));
        uint8_t r = 0, g = 0, b = 0;
        int ri = 0, gi = 0, bi = 0;
        ksscanf(r_str, "%d", &ri);
        ksscanf(g_str, "%d", &gi);
        ksscanf(b_str, "%d", &bi);
        if (ri < 0) ri = 0; else if (ri > 255) ri = 255;
        if (gi < 0) gi = 0; else if (gi > 255) gi = 255;
        if (bi < 0) bi = 0; else if (bi > 255) bi = 255;
        r = (uint8_t)ri; g = (uint8_t)gi; b = (uint8_t)bi;
        int ret = led_rgb(r, g, b);
        if (ret == 0) kprintf("  led rgb %d %d %d\n", r, g, b);
        return;
    }
    kprint("  led: unknown subcommand (set, pulse, rgb)\n");
}

/* Marky: Party network */
static void cmd_party(char *args) {
    char sub[32];
    const char *rest = next_word(args, sub, sizeof(sub));
    if (sub[0] == '\0') {
        kprint("  ");
        kprint_color("party", C_CYAN);
        kprint(" create [name] | invite <IP> | list | join [IP] | leave | chat <msg>\n");
        return;
    }
    party_init();
    if (ksstrcmp(sub, "create") == 0) {
        char name[PARTY_ROOM_NAME_MAX];
        next_word(rest, name, sizeof(name));
        int r = party_create(name[0] ? name : NULL);
        if (r == 0) kprint("  party: room created\n");
        else kprint("  party create failed\n");
        return;
    }
    if (ksstrcmp(sub, "invite") == 0) {
        char ip[PARTY_IP_STR_MAX];
        next_word(rest, ip, sizeof(ip));
        if (ip[0] == '\0') { kprint("  party invite <IP>\n"); return; }
        int r = party_invite(ip);
        if (r == 0) kprintf("  party: invited %s\n", ip);
        else kprint("  party invite failed\n");
        return;
    }
    if (ksstrcmp(sub, "list") == 0) {
        party_list();
        if (party_in_room()) {
            party_room_t room;
            party_get_room(&room);
            kprintf("  room: %s  members: %d\n", room.name[0] ? room.name : "(unnamed)", room.member_count);
        } else
            kprint("  no room (use party create or party join)\n");
        return;
    }
    if (ksstrcmp(sub, "join") == 0) {
        char ip[PARTY_IP_STR_MAX];
        next_word(rest, ip, sizeof(ip));
        int r = party_join(ip[0] ? ip : NULL);
        if (r == 0) kprint("  party: joined\n");
        else kprint("  party join failed\n");
        return;
    }
    if (ksstrcmp(sub, "leave") == 0) {
        party_leave();
        kprint("  party: left\n");
        return;
    }
    if (ksstrcmp(sub, "chat") == 0) {
        char msg[PARTY_CHAT_MSG_MAX];
        next_word(rest, msg, sizeof(msg));
        if (msg[0] == '\0') { kprint("  party chat <message>\n"); return; }
        int r = party_chat(msg);
        if (r == 0) kprint("  sent\n");
        else kprint("  party chat failed (in a room?)\n");
        return;
    }
    kprint("  party: unknown subcommand\n");
}

/* Marky: Streaming */
static void cmd_stream(char *args) {
    char sub[24];
    const char *rest = next_word(args, sub, sizeof(sub));
    if (sub[0] == '\0') {
        kprint("  ");
        kprint_color("stream", C_MAGENTA);
        kprint(" start <client_IP> | stop | status | quality <0|1|2>\n");
        return;
    }
    streaming_init();
    if (ksstrcmp(sub, "start") == 0) {
        char ip[PARTY_IP_STR_MAX];
        next_word(rest, ip, sizeof(ip));
        if (ip[0] == '\0') { kprint("  stream start <client_IP>\n"); return; }
        int r = streaming_start(ip);
        if (r == 0) kprintf("  stream: started -> %s\n", ip);
        else kprint("  stream start failed\n");
        return;
    }
    if (ksstrcmp(sub, "stop") == 0) {
        streaming_stop();
        kprint("  stream: stopped\n");
        return;
    }
    if (ksstrcmp(sub, "status") == 0) {
        if (streaming_active())
            kprint("  stream: active\n");
        else
            kprint("  stream: stopped\n");
        return;
    }
    if (ksstrcmp(sub, "quality") == 0) {
        char q[8];
        next_word(rest, q, sizeof(q));
        int v = 1;
        if (q[0] != '\0') ksscanf(q, "%d", &v);
        streaming_set_quality(v);
        kprintf("  stream quality: %d\n", v);
        return;
    }
    kprint("  stream: unknown subcommand\n");
}

/* Marky: Bluetooth */
static void cmd_bt(char *args) {
    char sub[24];
    const char *rest = next_word(args, sub, sizeof(sub));
    if (sub[0] == '\0') {
        kprint("  ");
        kprint_color("bt", C_BLUE);
        kprint(" scan | pair [addr] | unpair <addr> | list\n");
        return;
    }
    bt_init();
    if (ksstrcmp(sub, "scan") == 0) {
        int r = bt_scan();
        if (r == 0) kprint("  bt scan: no devices found\n");
        else kprintf("  bt scan: %d device(s) found\n", r);
        return;
    }
    if (ksstrcmp(sub, "pair") == 0) {
        char addr[BT_ADDR_STR_MAX];
        next_word(rest, addr, sizeof(addr));
        int r = bt_pair(addr[0] ? addr : "00:11:22:33:44:55");
        if (r == 0) kprint("  bt pair: ok\n");
        else kprint("  bt pair failed\n");
        return;
    }
    if (ksstrcmp(sub, "unpair") == 0) {
        char addr[BT_ADDR_STR_MAX];
        next_word(rest, addr, sizeof(addr));
        if (addr[0] == '\0') { kprint("  bt unpair <addr>\n"); return; }
        bt_unpair(addr);
        kprint("  bt unpair ok\n");
        return;
    }
    if (ksstrcmp(sub, "list") == 0) {
        bt_device_t devs[BT_DEVICE_MAX];
        int n = bt_list(devs, BT_DEVICE_MAX);
        if (n < 0) { kprint("  bt list failed\n"); return; }
        kprintf("  bt devices: %d\n", n);
        return;
    }
    kprint("  bt: unknown subcommand\n");
}

/* Marky: Controller mapping */
static void cmd_ctrlmap(char *args) {
    (void)args;
    bt_init();
    int r = controller_map_show();
    if (r == 0)
        kprint("  ctrlmap: identity mapping (default)\n");
    else
        kprint("  ctrlmap failed\n");
}

/* Marky: Controller profile */
static void cmd_ctrlprofile(char *args) {
    char sub[24];
    const char *rest = next_word(args, sub, sizeof(sub));
    if (sub[0] == '\0') {
        kprint("  ");
        kprint_color("ctrlprofile", C_YELLOW);
        kprint(" create <name> | load <name>\n");
        return;
    }
    bt_init();
    if (ksstrcmp(sub, "create") == 0) {
        char name[BT_PROFILE_NAME_MAX];
        next_word(rest, name, sizeof(name));
        if (name[0] == '\0') { kprint("  ctrlprofile create <name>\n"); return; }
        int r = bt_controller_profile_create(name);
        if (r == 0) kprintf("  profile created: %s\n", name);
        else kprint("  ctrlprofile create failed\n");
        return;
    }
    if (ksstrcmp(sub, "load") == 0) {
        char name[BT_PROFILE_NAME_MAX];
        next_word(rest, name, sizeof(name));
        if (name[0] == '\0') { kprint("  ctrlprofile load <name>\n"); return; }
        int r = bt_controller_profile_load(name);
        if (r == 0) kprintf("  profile loaded: %s\n", name);
        else kprint("  ctrlprofile load failed\n");
        return;
    }
    kprint("  ctrlprofile: unknown subcommand\n");
}

static void cmd_saves(char *args) {
    char sub[32], a1[32], a2[32];
    sub[0] = a1[0] = a2[0] = '\0';
    if (args && args[0] != '\0')
        ksscanf(args, "%31s %31s %31s", sub, a1, a2);
    if (ksstrcmp(sub, "list") == 0) {
        save_manager_list();
        return;
    }
    if (ksstrcmp(sub, "backup") == 0) {
        int slot = 0;
        if (a1[0]) ksscanf(a1, "%d", &slot);
        save_manager_backup(slot, a2[0] ? a2 : (a1[0] ? a1 : "backup"));
        return;
    }
    if (ksstrcmp(sub, "restore") == 0) {
        int slot = 0;
        if (a1[0]) ksscanf(a1, "%d", &slot);
        save_manager_restore(slot, a2[0] ? a2 : "backup");
        return;
    }
    if (ksstrcmp(sub, "clone") == 0 && a1[0] && a2[0]) {
        int dest = 0, src = 0;
        ksscanf(a1, "%d", &dest);
        ksscanf(a2, "%d", &src);
        save_manager_clone(dest, src);
        return;
    }
    if (ksstrcmp(sub, "versions") == 0) {
        int slot = 0;
        if (a1[0]) ksscanf(a1, "%d", &slot);
        save_manager_list_versions(slot);
        return;
    }
    if (ksstrcmp(sub, "rollback") == 0 && a1[0] && a2[0]) {
        int slot = 0, ver = 0;
        ksscanf(a1, "%d", &slot);
        ksscanf(a2, "%d", &ver);
        save_manager_rollback(slot, ver);
        return;
    }
    kprint("  ");
    kprint_color("saves", C_GREEN);
    kprint(" list|backup|restore|clone|versions|rollback\n");
    kprint("    list              list save candidates (FAT12)\n");
    kprint("    backup <slot> [n] backup to RAM\n");
    kprint("    restore <slot>    restore from backup\n");
    kprint("    clone <dest> <src> clone slot\n");
    kprint("    versions [slot]  list versions\n");
    kprint("    rollback <slot> <ver> restore version\n");
}

static void cmd_dashboard(char *args) {
    (void)args;
    dashboard_show();
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