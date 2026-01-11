#include "terminal.h"
#include "print.h"

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;
// char[VGA_WIDTH] line_buffer;

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
	terminal_color = vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
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

void terminal_scroll(bool prompt)
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
	terminal_column = 0;

	if (prompt){
		terminal_putchar('>'); // prompt
		terminal_column++;
		update_cursor();
	}
}

void terminal_putchar(char c) 
{
	// special handling for control characters
	switch(c) {
		
		//will add cache later for up/down arrows
		case -1: // Up arrow
			return;
		case -2: // Down arrow
			return;			

		case -3: // Left arrow
			if(terminal_column > 2){ // prevent moving before prompt
				terminal_column--;
				update_cursor();
			}
			return;

		case -4: // Right arrow
			if(terminal_column < VGA_WIDTH - 1){
				terminal_column++;
				update_cursor();
			}
			return;

		case '\b': // Backspace
			terminal_backspace();
			return;

		case '\n': // Newline
			terminal_column = 0;
			terminal_row++;
			terminal_putchar('>');
			terminal_column++;
				
			if (terminal_row == VGA_HEIGHT)
				terminal_scroll(true);

			// for (size_t x = 0; x < line_is_empty(terminal_row-1); x++) {
			// 	terminal_putchar(terminal_buffer[(terminal_row-1) * VGA_WIDTH + x] & 0xFF);
			// }

			update_cursor();
			return;

		case 9: // Tab
		// 4 spaces for tab
		for (int i = 0; i < 4; i++) {
			terminal_putchar(' ');
		}
			return;
	}

	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_scroll(false);
	}
	update_cursor();
}

void terminal_backspace(void)
{
    if (terminal_column == 0 && terminal_row == 12)
        return; // already at top-left

	// prevent deleting prompt
    if((terminal_buffer[terminal_row*VGA_WIDTH]&0xFF) != '>') { 

		// Move cursor back
        if (terminal_column == 0 ){
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
        }

		else {
			terminal_column--;
		}
    }

	else if (terminal_column>2){ // prevent deleting prompt
		terminal_column--;
	}

    // Clear the character under cursor
    terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
	update_cursor();
}

void update_cursor()
{
	uint16_t pos = terminal_row * VGA_WIDTH + terminal_column;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t)(pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

// Checks for the last empty column in line to send the user there when pressing up/down arrow
int line_is_empty(size_t row) {
	// value that represents the last empty column in the line
	int empty = 0;
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        if ((terminal_buffer[row * VGA_WIDTH + x] & 0xFF) != ' ')
            empty = x+1;// oomves the cursor behind the last character
    }
    return empty;
}

// checks that the user won't delete the first char in line ('>')
void check_zero_index(){
	if(terminal_column == 0){
		terminal_row --;
		terminal_column = line_is_empty(terminal_row);
	}
}