#include "Scanf.h"
#include "terminal.h"
#include "keyboard.h"

#define READ_STRING_MAX 256
#define READ_INT_MAX 32


int sys_read(int fd, char* buffer, uint32_t size);
int atoi(char *str);

int scanf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int bytes_read = 0;

    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '{') {

            if (fmt[i + 1] == 's' && fmt[i + 2] == '}') {
                // Read string
                char* str_buffer = va_arg(args, char*);
                bytes_read = sys_read(0, str_buffer, READ_STRING_MAX); // Read up to 256 bytes from stdin
                str_buffer[bytes_read] = '\0'; // Null-terminate the string
                i += 2; // Skip past 's}'
            }

            else if (fmt[i + 1] == 'd' && fmt[i + 2] == '}') {
                // Read integer
                int* int_ret = va_arg(args, int*);
                char int_buffer[32];
                int bytes_read = sys_read(0, int_buffer, 32); // Read up to 32 bytes for integer
                int_buffer[bytes_read] = '\0'; // Null-terminate the string
                *int_ret = atoi(int_buffer);
                i += 2; // Skip past 'd}'
            }
        }
    }

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

int atoi(char *str) {
    int k = 0;
    while (*str) {
        k = (k << 3) + (k << 1) + (*str) - '0';
        str++;
     }
     return k;
}