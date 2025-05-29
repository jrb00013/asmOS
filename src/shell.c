#include "include/shell.h"
#include "include/fs.h"
#include "include/syscalls.h"

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
    printf("\nPS2 x86 Shell\n");
    printf("Type 'help' for available commands\n");
}

void start_shell(void) {
    char input[128];
    char cmd[64];
    char args[64];
    
    while (1) {
        printf("> ");
        sys_read_line(input, sizeof(input));
        
        // Parse command and arguments
        if (sscanf(input, "%63s %63[^\n]", cmd, args) < 1) {
            continue;
        }
        
        // Find and execute command
        int found = 0;
        for (int i = 0; commands[i].name; i++) {
            if (strcmp(cmd, commands[i].name) == 0) {
                commands[i].func(args);
                found = 1;
                break;
            }
        }
        
        if (!found) {
            printf("Unknown command: %s\n", cmd);
        }
    }
}

// Command functions
static void cmd_help(char *args) {
    printf("Available commands:\n");
    for (int i = 0; commands[i].name; i++) {
        printf("  %s\n", commands[i].name);
    }
}

static void cmd_ls(char *args) {
    printf("Directory listing:\n");
    fat12_list_files();  // Uses our FAT12 implementation
}

static void cmd_meminfo(char *args) {
    uint32_t mem_kb;
    asm volatile("call get_memory_info" : "=a"(mem_kb));
    printf("Memory: %u KB available\n", mem_kb);
}

static void cmd_exit(char *args) {
    printf("Shutting down...\n");
    sys_exit(0);
}
