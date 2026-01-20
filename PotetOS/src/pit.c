#include "pit.h"
#include "io.h"
#include "scheduler.h"

/* PIT runs at input clock 1193180 Hz */
#define PIT_INPUT_FREQ 1193180
#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40

static volatile uint32_t ticks = 0;

void pit_init(uint32_t frequency)
{
    if (frequency == 0) return;

    uint32_t divisor = PIT_INPUT_FREQ / frequency;

    /* Command: channel 0, access mode lobyte/hibyte, mode 2 (rate generator), binary */
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}

void pit_handle_tick(void)
{
    ticks++;
    scheduler_tick();
}

uint32_t pit_get_ticks(void)
{
    return ticks;
}
