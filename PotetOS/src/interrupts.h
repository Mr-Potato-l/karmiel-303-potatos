#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>
#include "print.h"

void isr_common_handler(uint32_t vector_number);
void irq_common_handler(uint32_t irq);
char* inttoa(int value, char* str);

#endif