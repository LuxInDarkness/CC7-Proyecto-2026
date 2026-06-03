#include "os_io.h"

#define INT_TO_STRING_BUFFER_SIZE 32
#define FLOAT_TO_STRING_BUFFER_SIZE 32
#define PRINT_BUFFER_SIZE 256

// Function to send a string via UART
void os_write(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

void vprint(const char *s, va_list args) {
    int mod_flag = 0;
    char final[PRINT_BUFFER_SIZE];
    char *final_ptr = final;
    char *final_end = final + PRINT_BUFFER_SIZE - 1;

    while (*s && final_ptr < final_end) {
        if (*s == '%') {
            mod_flag = 1;
        } else if (mod_flag) {
            if (*s == 's') {
                char *curr_str = va_arg(args, char*);
                if (curr_str) {
                    while (*curr_str && final_ptr < final_end) {
                        *final_ptr++ = *curr_str++;
                    }
                }
            } else if (*s == 'd') {
                char buffer[INT_TO_STRING_BUFFER_SIZE];
                int curr_int = va_arg(args, int);
                char *curr_str = int2alpha(curr_int, buffer);
                while (*curr_str && final_ptr < final_end) {
                    *final_ptr++ = *curr_str++;
                }
            } else if (*s == 'f') {
                char buffer[FLOAT_TO_STRING_BUFFER_SIZE];
                float curr_float = (float)va_arg(args, double);
                char *curr_str = float2alpha(curr_float, buffer);
                while (*curr_str && final_ptr < final_end) {
                    *final_ptr++ = *curr_str++;
                }
            } else {
                if (final_ptr + 1 < final_end) {
                    *final_ptr++ = '%';
                    *final_ptr++ = *s;
                }
            }
            mod_flag = 0;
        } else {
            *final_ptr++ = *s;
        }
        s++;
    }

    *final_ptr = '\0';
    os_write(final);
}

void print(const char *s, ...) {
    va_list args;
    va_start(args, s);
    vprint(s, args);
    va_end(args);
}
