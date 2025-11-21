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

		// else if (strcmp(spec, "x") == 0)
		// 	print_hex(va_arg(args, unsigned int));
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