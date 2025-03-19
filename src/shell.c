#include "include/shell.h"
#include "include/uart.h"

// Simple shell that accepts commands and prints a prompt
void init_shell(void) {
    // Initialize UART for input/output
    init_uart();
    printf("Welcome to the Simple Shell!\n");

    // Start shell command prompt
    start_shell();
}

void start_shell(void) {
    char input[100];
    
    while (1) {
        printf("> ");
        fgets(input, 100, stdin);  // Get user input from UART (or keyboard in real OS)

        // Parse and execute the command
        if (strcmp(input, "exit\n") == 0) {
            printf("Exiting shell...\n");
            break;
        }
        else {
            printf("Unknown command: %s\n", input);
        }
    }
}
