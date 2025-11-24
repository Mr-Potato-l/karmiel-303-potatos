#include "terminal.h"

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = (uint16_t*) 0xB8000;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_scroll(void)
{
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			if(y != VGA_HEIGHT - 1)
			{
				const size_t next_index = (y + 1) * VGA_WIDTH + x;
				terminal_buffer[index] = terminal_buffer[next_index];
			}
			else 
				terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
	terminal_row = VGA_HEIGHT - 1;
}

void terminal_putchar(char c) 
{
	if(c == '\n') {
		terminal_column = 0;
		terminal_row++;
		
		if (terminal_row == VGA_HEIGHT)
        	terminal_scroll();
        
		return;
	}
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_scroll();
	}
}

void terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) 
{
	terminal_write(data, strlen(data));
}

void terminal_backspace(void)
{
    if (terminal_column == 0 && terminal_row == 5)
        return; // already at top-left

    // Move cursor back
    if (terminal_column == 0) {
        terminal_row--;
        terminal_column = VGA_WIDTH - 1;

        // Jump over trailing spaces backwards
        size_t index = terminal_row * VGA_WIDTH + terminal_column;
        while (index > 0 && (terminal_buffer[index] & 0xFF) == ' ' && terminal_column > 0) {
            index--;
            terminal_column--;
            if (terminal_column >= VGA_WIDTH) { // handle wrap-around
                terminal_column = VGA_WIDTH - 1;
                terminal_row--;
            }
        }
    } else {
        terminal_column--;
    }

    

    // Clear the character under cursor
    terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
}

