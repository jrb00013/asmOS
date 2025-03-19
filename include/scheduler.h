#ifndef SCHEDULER_H
#define SCHEDULER_H

// Define the maximum number of tasks that can be scheduled
#define MAX_TASKS 5

// Declare the task scheduling functions
void add_task(void (*task_func)(void));  // Add a task to the scheduler
void run_scheduler(void);                // Run the round-robin scheduler
void init_scheduler(void);

#endif  // SCHEDULER_H
