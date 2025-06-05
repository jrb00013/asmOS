#include "shell.h"
#include "fs.h"
#include "syscalls.h"
#include "kernel.h"

// Shell commands structure
typedef struct {
    const char *name;
    void (*func)(char *args);
} shell_command_t;

// Command implementations
static void cmd_help(char *args);
static void cmd_ls(char *args);
static void cmd_meminfo(char *args);
static void cmd_exit(char *args);

// Available commands
static shell_command_t commands[] = {
    {"help", cmd_help},
    {"ls", cmd_ls},
    {"meminfo", cmd_meminfo},
    {"exit", cmd_exit},
    {NULL, NULL}
};

void init_shell(void) {
    kprint("\nPS2 x86 Shell\n");
    kprint("Type 'help' for available commands\n");
}

void start_shell(void) {
    char input[128];
    char cmd[64];
    char args[64];
    
    while (1) {
        kprint("> ");
        sys_read_line(input, sizeof(input));
        
        // Parse command and arguments
        if (ksscanf(input, "%63s %63[^\n]", cmd, args) < 1) {
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
        }
    }
}

// Command functions
static void cmd_help(char *args) {
    kprint("Available commands:\n");
    for (int i = 0; commands[i].name; i++) {
        kprintf("  %s\n", commands[i].name);
    }
}

static void cmd_ls(char *args) {
    kprint("Directory listing:\n");
    fat12_list_files();  // Uses our FAT12 implementation
}

static void cmd_meminfo(char *args) {
    uint32_t mem_kb;
    asm volatile("call get_memory_info" : "=a"(mem_kb));
    kprintf("Memory: %u KB available\n", mem_kb);
}

static void cmd_exit(char *args) {
    kprint("Shutting down...\n");
    sys_exit(0);
}
