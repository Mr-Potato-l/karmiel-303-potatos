#include "print.h"

void print(const char* txt, ...)
{
	va_list args;
    va_start(args, txt);
    
	
    for (int i = 0; txt[i]; i++) {	
        
        if (txt[i] == '{') {
            if (txt[i+1] == '{') {
                terminal_putchar('{');
                i++;
                continue;
            }

		// Read until '}'
		// int si = 0;
		char spec;
		i++;

		// IN CASE WE WANT TO ADD A SPECIFIER THAT IS LONGER THAN ONE:
		//
		// while (txt[i] != '}' && txt[i] != '\0') {
		// 	spec[si++] = txt[i++];
		// }
		//
		// spec[si] = '\0';

		spec = txt[i];
		i++;

		// 	MORE PRINTING OPTIONS THAT WILL APPEAR LATER:
		
		// Now process specifier
		if (spec == 'd')
			print_int(va_arg(args, int));

		else if (spec == 'x')
			print_hex(va_arg(args, unsigned int));

		else if (spec == 'f')
			print_float(va_arg(args, double));

		else if (spec =='s')
			print_string(va_arg(args, char*));

		else if (spec == 'c')
			terminal_putchar(va_arg(args, int)); // char promotes to int

		continue;
        }

        // Handle normal character
        terminal_putchar(txt[i]);
    }

    va_end(args);
}

void print_int(int value)
{
	if (value == 0) {
		terminal_putchar('0');
		return;
	}

	if (value < 0) {
		terminal_putchar('-');
		value = -value;
	}

	char buffer[10]; // Enough for 32-bit int
	int i = 0;

	// Extract digits (in reverse order)
	while (value > 0) {
		buffer[i++] = (value % 10) + '0';
		value /= 10;
	}

	// Print the digits in reverse order
	for (int j = i - 1; j >= 0; j--) {
		terminal_putchar(buffer[j]);
	}
}

void print_string(const char* str)
{
	while (*str != '\0') {
		terminal_putchar(*str++);
	}
}

void print_hex(unsigned int value)
{
	if (value == 0) {
		terminal_putchar('0');
		return;
	}

	char buffer[8]; // Enough for 32-bit hex
	int i = 0;

	while (value > 0) {
		unsigned int digit = value & 0xF;
		if (digit < 10)
			buffer[i++] = digit + '0';
		else
			buffer[i++] = digit - 10 + 'A';
		value >>= 4;
	}

	// Print the digits in reverse order
	for (int j = i - 1; j >= 0; j--) {
		terminal_putchar(buffer[j]);
	}
}

void print_float(float value)
{
	if (value == 0.0f) {
		print("0.0");
		return;
	}

	if (value < 0) {
		terminal_putchar('-');
		value = -value;
	}

	int count = 0;
	int int_part = (int)value;
	print_int(int_part);

	terminal_putchar('.');


	// moving the number one decimal place to the left 
	float frac_part = (value - int_part) * 10;

	// printing the fractional part (one digit at a time)
	// limiting to 6 digits after the point
	while (frac_part != 0.0f && count < 6) {
		print_int((int)frac_part);

		frac_part = (frac_part - (int)frac_part) * 10;
		count++;
	} 
}