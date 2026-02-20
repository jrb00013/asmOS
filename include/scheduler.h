#ifndef SCHEDULER_H
#define SCHEDULER_H

// Define the maximum number of tasks that can be scheduled
#define MAX_TASKS 8

// Task ID (0 .. MAX_TASKS-1), or -1 if not a task
typedef int task_id_t;

// Declare the task scheduling functions
void add_task(void (*task_func)(void));  // Add a task to the scheduler
void run_scheduler(void);                // Run the round-robin scheduler
void init_scheduler(void);

/* Multitasking workaround: cooperative yield. No native kernel — we are the kernel. */
void task_yield(void);                   // Yield to next task (saves esp, loads next, returns in other task)
task_id_t task_current(void);            // Current task index, or -1 if not in a task

#endif  // SCHEDULER_H
