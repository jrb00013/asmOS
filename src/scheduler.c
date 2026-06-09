#include "scheduler.h"
#include "msp.h"
#include "kernel.h"
#include "memory_manager.h"
#include "arch_x86.h"
#include <stdint.h>

static struct {
    void (*func)(void);
    uint32_t esp;  /* Stack pointer for context switching */
} task_list[MAX_TASKS];

static int task_count = 0;
static int current_task = -1;  /* -1 until first yield or run */

void add_task(void (*task_func)(void)) {
    if (task_count < MAX_TASKS) {
        uint8_t *stack = (uint8_t*)malloc(1024);
        if (!stack) return;
        task_list[task_count].func = task_func;
        /* Stack: high addr = top. We need space for pusha (32 bytes) + ret addr (4). */
        task_list[task_count].esp = (uint32_t)(stack + 1024 - 36);
        uint32_t *stack_top = (uint32_t*)(stack + 1024 - 36);
        stack_top[0] = 0; stack_top[1] = 0; stack_top[2] = 0; stack_top[3] = 0;
        stack_top[4] = 0; stack_top[5] = 0; stack_top[6] = 0; stack_top[7] = 0;  /* pusha placeholder */
        stack_top[8] = (uint32_t)task_func;  /* return addr = task entry */
        task_count++;
    }
}

task_id_t task_current(void) {
    return (task_id_t)current_task;
}

#ifdef PLATFORM_PS2
/* MIPS EE: cooperative round-robin without x86 context switch. */
void task_yield(void) {
    if (task_count <= 0) return;
    if (current_task < 0) current_task = 0;
    current_task = (current_task + 1) % task_count;
    if (task_list[current_task].func)
        task_list[current_task].func();
}

void run_scheduler(void) {
    if (task_count > 0 && task_list[0].func)
        task_list[0].func();
}
#else
/* Save current esp, switch to next task's esp, popa, ret. Cooperative multitasking. */
void task_yield(void) {
    if (task_count <= 0) return;
    if (current_task < 0) current_task = 0;
    int next = (current_task + 1) % task_count;
    uint32_t *save_to = &task_list[current_task].esp;
    uint32_t load_esp = task_list[next].esp;
    current_task = next;
    task_yield_asm(save_to, load_esp);
}

void run_scheduler(void) {
    if (task_count <= 0) return;
    current_task = 0;
    run_scheduler_asm(task_list[0].esp);
    while (1) task_yield();
}
#endif

void init_scheduler(void) {
    kprintf("Scheduler: %d tasks registered\n", task_count);
}
