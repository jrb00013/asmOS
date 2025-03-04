#include "msp.h"
#include "shell.h"
#include "memory_manager.h"
#include "scheduler.h"

// Initialize kernel components
void init_kernel(void) {
    init_memory_manager();
    init_scheduler();
    init_shell();
}

// Main kernel entry point
void kernel_main(void) {
    // Initialize kernel
    init_kernel();

    // Print a simple message to indicate kernel is running
    printf("Kernel Loaded!\n");

    // Enter shell (command line interface)
    start_shell();
}
