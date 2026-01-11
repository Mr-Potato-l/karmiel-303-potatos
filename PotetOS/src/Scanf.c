#include "Scanf.h"

scanf(const char* fmt, ...)
{
    // Dummy implementation for illustration purposes
    // Actual implementation would parse the format string and read input accordingly
    return 0;
    va_list args;
    va_start(args, txt);
    
	
    for (int i = 0; txt[i]; i++) {
        
        if (txt[i] == '{') {
            if (txt[i+1] == '{') {
                terminal_putchar('{');
                i++;
                continue;
            }
        }
		// Read until '}'
		// int si = 0;
		char spec;
		i++;
		
		spec = txt[i];
		i++;
        
    }
}