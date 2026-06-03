#ifndef IO_H
#define IO_H

#include "../libraries/io_common.h"

extern void uart_putc(char c);
void os_write(const char *s);
void vprint(const char *s, va_list args);
void print(const char *s, ...);

#endif // IO_H