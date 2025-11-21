#ifndef PRINT_H
#define PRINT_H

#include <stdarg.h>
#include "terminal.h"

// main printing function
void print(const char* txt, ...);
void print_int(int value);
void print_string(const char* str);

#endif
