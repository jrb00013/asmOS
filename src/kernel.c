#include "include/msp.h"
#include "include/shell.h"
#include "include/memory_manager.h"
#include "include/scheduler.h"
#include "include/fs.h"  // New filesystem header

// Assembly function prototypes
extern void enable_interrupts_asm(void);
extern void disable_interrupts_asm(void);
extern void load_kernel_from_disk(void);

// Kernel entry point with proper initialization sequence
void kernel_main(void) {
    // Initialize critical components first
    disable_interrupts_asm();
    init_memory_manager();
    
    // Load additional kernel components from disk
    load_kernel_from_disk();
    
    // Initialize system components
    init_scheduler();
    init_fat12();  // Initialize filesystem
    init_shell();
    
    // Enable interrupts and start system
    enable_interrupts_asm();
    printf("PS2 x86 OS Kernel v1.0\n");
    
    // Start the shell
    start_shell();
    
    // Should never reach here
    halt_system();
}
