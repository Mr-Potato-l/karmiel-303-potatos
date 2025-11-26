#include "interrupts.h"
#include "keyboard.h"
#include "pic.h"

void isr_common_handler(uint32_t vector_number) {
    print("Interrupt ");

    char buff[16];
    print(inttoa(vector_number, buff));

    switch (vector_number)
    {
        case  0:
            print(": Division By Zero\n");
            break;
        
        default:
            print(": Unknown Interrupt\n");
            break;
    }

    while (1) {
        __asm__ __volatile__("hlt");
    }
}

void irq_common_handler(uint32_t irq)
{
    // print("* waiting for input *\n");
    switch (irq)
    {
        case 0:
            break;
        case 1:
            //print("Keyboard IRQ\n");
            keyboard_handler();
            break;
        default:
            print("Unknown IRQ\n");
            break;
    }

    PIC_sendEOI(irq);
}

char* inttoa(int value, char* buffer) {
    int i = 0;
    int isNegative = 0;

    // Handle 0 explicitly
    if (value == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return buffer;
    }

    // Handle negative numbers
    if (value < 0) {
        isNegative = 1;
        value = -value;
    }

    // Convert digits
    while (value != 0) {
        int digit = value % 10;
        buffer[i++] = '0' + digit;
        value /= 10;
    }

    // Add minus sign if needed
    if (isNegative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }

    return buffer;
}
