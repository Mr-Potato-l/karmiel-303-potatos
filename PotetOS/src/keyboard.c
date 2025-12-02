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
    

// void keyboard_handler() {
//     uint8_t scancodeancode = inb(KBD_DATA);

//     // ignore release codes (>= 0x80)
//     if (scancodeancode < 128) {
//         char c = kbd_us[scancodeancode];
//         print("Key pressed: {c}\n", c);
//         if(c == '\b') 
//             terminal_backspace();
//         else if (c != 0)
//             terminal_putchar(c);
//     }
// }

void keyboard_handler() {
    uint8_t scancode = inb(KBD_DATA);
    static int extended = 0;

    // Check for extended scancodeancode prefix
    if (scancode == 0xE0) {
        extended = 1;
        return;
    }

    // Handle extended scancodeancode
    if (extended) {
        extended = 0;

        switch (scancode) {
            case 0x48: // Arrow Up
                terminal_putchar(-1); // Indicate up arrow
                break;
            case 0x50: // Arrow Down
                terminal_putchar(-2); // Indicate down arrow
                break;
            case 0x4B: // Arrow Left
                terminal_putchar(-3); // Indicate left arrow
                break;
            case 0x4D: // Arrow Right
                terminal_putchar(-4); // Indicate right arrow
                break;
        }
        return;
    }

    // ignore release codes (>= 0x80)
    if (scancode < 128) {
        char c = kbd_us[scancode];
        if (c != 0)
            terminal_putchar(c);
    }
}