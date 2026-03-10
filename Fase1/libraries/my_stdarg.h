#ifndef MY_STDARG_H
#define MY_STDARG_H

// va_list is just a pointer to the argument area on the stack
typedef unsigned char *va_list;

// Align size up to 4 bytes (ARM word size)
#define VA_ALIGN(type) \
    ((sizeof(type) + sizeof(int) - 1) & ~(sizeof(int) - 1))

// Start: point just past the last named argument
// We take the address of the last named param and step over it
#define va_start(ap, last) \
    ((ap) = (va_list)(&(last)) + VA_ALIGN(last))

// Fetch next arg and advance the pointer
#define va_arg(ap, type) \
    (*(type *)((ap) += VA_ALIGN(type), (ap) - VA_ALIGN(type)))

// Nothing to clean up on ARM bare metal
#define va_end(ap) ((void)0)

#endif // MY_STDARG_H