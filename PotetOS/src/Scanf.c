#include "Scanf.h"
#include "terminal.h"
#include "keyboard.h"

int sys_read(int fd, char* buffer, uint32_t size);

int scanf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char* str_buffer = va_arg(args, char*);
    int bytes_read = sys_read(0, str_buffer, 256); // Read up to 256 bytes from stdin
    str_buffer[bytes_read] = '\0'; // Null-terminate the string

    va_end(args);
    return bytes_read;
}

int sys_read(int fd, char* buffer, uint32_t size) {
    if (fd != 0) {
        return -1; // Only stdin (fd 0) is supported
    }

    uint32_t i = 0;
    for (i = 0; i < size - 1; i++) {
        char c = getchar();
        
        // Stop reading on newline
        if (c == '\n') {
            break; 
        }

        buffer[i] = c;
    }
    buffer[i] = '\0'; // Null-terminate the string
    return i; // Return number of bytes read
}