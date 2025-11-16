#include "print.h"

void putchar(char c) {
  extern long write(int, const char *, unsigned long);
  (void) write(1, &c, 1);
}

int print(const *char txt, ...)
{
	va_list args;
    va_start(args, txt);
    
	
    for (int i = 0; txt[i]; i++) {	
        /*
        if (txt[i] == '{') {
            if (txt[i+1] == '{') {
                putchar('{');
                i++;
                continue;
            }

		// Read until '}'
		char spec[16];
		int si = 0;
		i++;

		while (txt[i] != '}' && txt[i] != '\0') {
			spec[si++] = txt[i++];
		}
		
		spec[si] = '\0';

		// Now process specifier
		if (strcmp(spec, "d") == 0)
			print_int(va_arg(args, int));
		else if (strcmp(spec, "x") == 0)
			print_hex(va_arg(args, unsigned int));
		else if (strcmp(spec, "s") == 0)
			print_string(va_arg(args, char*));
		else if (strcmp(spec, "c") == 0)
			putchar((char)va_arg(args, int)); // char promotes to int

		continue;
        }
        */

        // Handle normal character
        putchar(txt[i]);
    }

    va_end(args);
}
