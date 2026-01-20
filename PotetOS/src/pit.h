#ifndef PIT_H
#define PIT_H

#include <stdint.h>

/* Initialize the PIT to the given frequency (Hz) */
void pit_init(uint32_t frequency);

/* Called from the timer IRQ to advance the tick counter */
void pit_handle_tick(void);

/* Retrieve number of ticks since boot */
uint32_t pit_get_ticks(void);

#endif
