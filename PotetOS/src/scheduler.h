#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#define MAX_TASKS 16

typedef void (*task_fn_t)(void);

void scheduler_init(void);
int scheduler_create(task_fn_t fn);
void scheduler_tick(void);
void scheduler_run_pending(void);
void scheduler_yield(void);

#endif
