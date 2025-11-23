#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>
#include "terminal.h"

void isr_common_handler(uint32_t vector_number);
void itoa(int value, char* str, int base);

#endif