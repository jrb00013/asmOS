#include "include/scheduler.h"

#define MAX_TASKS 5
static void (*task_list[MAX_TASKS])(void);  // Array of task functions
static int task_count = 0;

// Function to add tasks to the scheduler
void add_task(void (*task_func)(void)) {
    if (task_count < MAX_TASKS) {
        task_list[task_count++] = task_func;  // Add task to the list
    }
}

// Simple round-robin task scheduler
void run_scheduler(void) {
    int current_task = 0;

    // Run tasks in a round-robin fashion
    while (1) {
        task_list[current_task]();  // Execute the current task
        current_task = (current_task + 1) % task_count;  // Move to the next task
    }
}
