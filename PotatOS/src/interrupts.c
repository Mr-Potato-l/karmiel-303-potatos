#include "interrupts.h"

void isr_common_handler(uint32_t vector_number) {
    terminal_writestring("Interrupt: ");

    switch (vector_number)
    {
        case  0:
            terminal_writestring("Division By Zero\n");
            break;
        
        default:
            terminal_writestring("Unknown Interrupt\n");
            break;
    }

    while (1) {
        __asm__ __volatile__("hlt");
    }
}
