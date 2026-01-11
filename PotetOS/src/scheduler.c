#include "scheduler.h"
#include "print.h"
#include <stddef.h>

extern void context_switch(uint32_t **old_sp, uint32_t *new_sp);

#define STACK_SIZE 4096

typedef struct {
    uint32_t *stack_ptr;
    uint32_t stack[STACK_SIZE / 4];
    task_fn_t fn;
    int active;
} task_t;

static task_t tasks[MAX_TASKS];
static int task_count = 0;
/* current == -1 means kernel is running */
static int current = -1;
static volatile int pending = 0;
static uint32_t *kernel_stack = NULL;

static void task_start(void)
{
    if (current < 0 || current >= task_count) return;
    if (tasks[current].fn) tasks[current].fn();
    tasks[current].active = 0;
    /* When task returns, yield to next */
    scheduler_yield();
}

void scheduler_init(void)
{
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].stack_ptr = NULL;
        tasks[i].fn = NULL;
        tasks[i].active = 0;
        /* stack contents are uninitialized */
    }
    task_count = 0;
    current = -1;
    pending = 0;
    kernel_stack = NULL;
}

int scheduler_create(task_fn_t fn)
{
    if (task_count >= MAX_TASKS) return -1;
    task_t *t = &tasks[task_count];
    t->fn = fn;
    t->active = 1;

    /* Prepare initial stack: layout must match popad/pusha and a return address */
    uint32_t *sp = &t->stack[(STACK_SIZE / 4)];
    /* reserve space for EDI,ESI,EBP,ESP,EBX,EDX,ECX,EAX (popad will pop into these) + return addr */
    sp -= 9;
    for (int i = 0; i < 8; i++) sp[i] = 0;
    sp[8] = (uint32_t)task_start; /* return address */
    t->stack_ptr = sp;

    task_count++;
    return task_count - 1;
}

void scheduler_tick(void)
{
    /* mark that scheduler should run on next opportunity */
    pending = 1;
}

void scheduler_run_pending(void)
{
    if (!pending) return;
    pending = 0;

    if (task_count == 0) return;

    /* find next active task */
    int start = current;
    int next = current;
    do {
        next = (next + 1) % task_count;
        if (tasks[next].active) break;
    } while (next != start);

    if (!tasks[next].active) return; /* no runnable tasks */

    int prev = current;
    current = next; /* set current so task_start sees it */

    if (prev == -1) {
        /* save kernel stack pointer once */
        if (!kernel_stack) {
            uint32_t esp;
            asm volatile ("movl %%esp, %0" : "=r" (esp));
            kernel_stack = (uint32_t*)esp;
        }
        context_switch(&kernel_stack, tasks[next].stack_ptr);
    } else {
        context_switch(&tasks[prev].stack_ptr, tasks[next].stack_ptr);
    }
}

void scheduler_yield(void)
{
    if (task_count == 0) return;
    int prev = current;

    /* find next active task */
    int start = current;
    int next = current;
    do {
        next = (next + 1) % task_count;
        if (tasks[next].active) break;
    } while (next != start);

    /* If no other runnable task, switch back to kernel if we're in a task */
    if (next == current) {
        if (prev != -1 && kernel_stack) {
            int old = current;
            current = -1;
            context_switch(&tasks[old].stack_ptr, kernel_stack);
        }
        return;
    }

    current = next;
    context_switch(&tasks[prev].stack_ptr, tasks[next].stack_ptr);
}
