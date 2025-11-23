#include "keyboard.h"
#include "terminal.h"
#include "io.h"

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
        if (c != 0)
            terminal_putchar(c);
    }
}
