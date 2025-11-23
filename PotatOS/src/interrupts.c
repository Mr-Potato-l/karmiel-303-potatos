#include "interrupts.h"
#include "keyboard.h"
#include "pic.h"

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

void irq_common_handler(uint32_t vector_number)
{
    uint8_t irq = vector_number;

    switch (irq)
    {
        case 1:
            terminal_writestring("Keyboard Interrupt\n");
            // keyboard_handler();
            break;
        default:
            terminal_writestring("Unknown IRQ\n");
            break;
    }

    PIC_sendEOI(irq);
}
