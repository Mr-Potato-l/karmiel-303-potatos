#include "keyboard.h"
#include "terminal.h"
#include "io.h"
#include "interrupts.h"

#define KBD_STATUS 0x64
#define KBD_DATA   0x60
#define KBD_OBF    0x01 // Output buffer full

unsigned char kbd_us[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t', 'q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0, ' '
};
    

void keyboard_handler() {
    uint8_t scancode = inb(0x60);

    // ignore release codes (>= 0x80)
    if (scancode < 128) {
        char c = kbd_us[scancode];
        if(c == '\b') 
            terminal_backspace();
        else if (c != 0)
            terminal_putchar(c);
    }
}
