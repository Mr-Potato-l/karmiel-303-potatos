#include "scheduler.h"
#include "print.h"
#include <stddef.h>

static task_fn_t tasks[MAX_TASKS];
static int task_count = 0;
static int current = 0;
static volatile int pending = 0;

void scheduler_init(void)
{
    for (int i = 0; i < MAX_TASKS; i++) tasks[i] = NULL;
    task_count = 0;
    current = 0;
    pending = 0;
}

int scheduler_create(task_fn_t fn)
{
    if (task_count >= MAX_TASKS) return -1;
    tasks[task_count++] = fn;
    return task_count - 1;
}

void scheduler_tick(void)
{
    if (task_count == 0) return;
    current = (current + 1) % task_count;
    pending = 1;
}

void scheduler_run_pending(void)
{
    if (!pending) return;
    pending = 0;
    if (tasks[current]) {
        tasks[current]();
    }
}
