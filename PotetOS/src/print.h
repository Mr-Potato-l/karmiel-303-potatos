#ifndef PRINT_H
#define PRINT_H

#include <stdarg.h>
#include <stdint.h>

// main printing function
void print(const char* txt, ...);
void print_int(int value);
void print_string(const char* str);
void print_hex(unsigned int value);
void print_uint64(uint64_t addr);
void print_float(float value);

#endif
