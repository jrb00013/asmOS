#include "shell.h"
#include "fs.h"
#include "syscalls.h"
#include "kernel.h"

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

// Enhanced command list with PS2-specific commands
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
    {"exit", cmd_exit, "Exit shell"},
    {NULL, NULL, NULL}
};

void init_shell(void) {
    kprint("\nPS2 x86 Enhanced Shell v2.0\n");
    kprint("Type 'help' for available commands\n");
    kprint("Type 'ps2info' for PS2 hardware information\n");
}

void start_shell(void) {
    char input[256];  // Increased buffer size
    char cmd[64];
    char args[192];
    
    while (1) {
        print_prompt();
        sys_read_line(input, sizeof(input));
        
        // Parse command and arguments
        if (ksscanf(input, "%63s %191[^\n]", cmd, args) < 1) {
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
            kprintf("Unknown command: %s\n", cmd);
            kprint("Type 'help' for available commands\n");
        }
    }
}

// Enhanced command functions
static void cmd_help(char *args) {
    kprint("PS2 x86 OS - Available Commands:\n");
    kprint("================================\n");
    for (int i = 0; commands[i].name; i++) {
        kprintf("  %-12s - %s\n", commands[i].name, commands[i].description);
    }
    kprint("\nPS2-specific commands: ps2info, reboot\n");
}

static void cmd_ls(char *args) {
    kprint("Directory listing:\n");
    kprint("==================\n");
    fat12_list_files();  // Uses our FAT12 implementation
}

static void cmd_cd(char *args) {
    if (ksstrcmp(args, "") == 0) {
        kprint("Current directory: /\n");
    } else {
        kprintf("Changing to directory: %s\n", args);
        // TODO: Implement directory navigation
        kprint("Directory navigation not yet implemented\n");
    }
}

static void cmd_cat(char *args) {
    if (ksstrcmp(args, "") == 0) {
        kprint("Usage: cat <filename>\n");
        return;
    }
    
    kprintf("Displaying file: %s\n", args);
    // TODO: Implement file reading
    kprint("File reading not yet implemented\n");
}

static void cmd_clear(char *args) {
    // Clear screen
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = ((uint16_t)DEFAULT_COLOR << 8) | ' ';
    }
    cursor_row = 0;
    cursor_col = 0;
}

static void cmd_date(char *args) {
    // Get current date from BIOS
    uint16_t year, month, day;
    uint16_t hour, minute, second;
    
    // This would need BIOS calls to get real date/time
    kprint("Date/Time: 2024-01-01 12:00:00 (PS2 System Time)\n");
}

static void cmd_echo(char *args) {
    if (ksstrcmp(args, "") != 0) {
        kprintf("%s\n", args);
    }
}

static void cmd_reboot(char *args) {
    kprint("Rebooting PS2 system...\n");
    for (volatile int i = 0; i < 1000000; i++); // Delay
    // Trigger system reset
    asm volatile("int $0x19"); // BIOS reboot
}

static void cmd_meminfo(char *args) {
    uint32_t mem_kb;
    asm volatile("call get_memory_info" : "=a"(mem_kb));
    kprintf("Memory Information:\n");
    kprintf("==================\n");
    kprintf("Total Memory: %u KB (%u MB)\n", mem_kb, mem_kb / 1024);
    kprintf("Available Memory: %u KB\n", mem_kb - 1024); // Reserve 1MB for system
    kprintf("Memory Type: PS2 DDR SDRAM\n");
}

static void cmd_ps2info(char *args) {
    kprint("PS2 Hardware Information:\n");
    kprint("=========================\n");
    kprint("System: PlayStation 2 (PS2)\n");
    kprint("CPU: MIPS R5900 (Emotion Engine)\n");
    kprint("GPU: Graphics Synthesizer\n");
    kprint("Memory: 32MB DDR SDRAM\n");
    kprint("Storage: CD/DVD Drive\n");
    kprint("Controllers: DualShock 2 Support\n");
    kprint("Network: Ethernet (optional)\n");
    kprint("OS: PS2 x86 Assembly Real-Mode OS v2.0\n");
    kprint("Boot Method: Modchip (DMS3)\n");
}

static void cmd_exit(char *args) {
    kprint("Shutting down PS2 x86 OS...\n");
    kprint("Thank you for using PS2 x86 OS!\n");
    sys_exit(0);
}

void print_prompt(void) {
    kprint_color("PS2OS ", 0x2A);  // light green
    kprint_color("[user@ps2]:~$ ", 0x1F);  // light blue on white
}