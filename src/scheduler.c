#include "scheduler.h"
#include "msp.h"
#include "kernel.h"
#include "memory_manager.h"


static struct {
    void (*func)(void);
    uint32_t esp;  // Stack pointer for context switching
} task_list[MAX_TASKS];

static int task_count = 0;
static int current_task = 0;

void add_task(void (*task_func)(void)) {
    if (task_count < MAX_TASKS) {
        // Allocate stack for the task
        uint8_t *stack = (uint8_t*)malloc(1024);
        if (!stack) return;
        
        // Initialize task context
        task_list[task_count].func = task_func;
        task_list[task_count].esp = (uint32_t)(stack + 1024 - 16);
        
        // Set up initial stack frame
        uint32_t *stack_top = (uint32_t*)(stack + 1024 - 16);
        stack_top[-1] = (uint32_t)task_func;  // EIP
        stack_top[-2] = 0x202;               // EFLAGS (interrupts enabled)
        
        task_count++;
    }
}

void run_scheduler(void) {
    while (1) {
        // Switch to next task
        current_task = (current_task + 1) % task_count;
        
        // Assembly context switch
        asm volatile(
            "mov %0, %%esp\n\t"  // Load new stack
            "popa\n\t"           // Restore registers
            "iret"               // Return to new task
            : 
            : "r"(task_list[current_task].esp)
            : "memory"
        );
    }
}

void init_scheduler(void) {
    kprintf("Scheduler: %d tasks registered\n", task_count);
}
