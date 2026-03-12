#include "io.h"
#include "my_stdarg.h"

#define INT_TO_STRING_BUFFER_SIZE 32
#define FLOAT_TO_STRING_BUFFER_SIZE 32
#define PRINT_BUFFER_SIZE 256

// Function to send a string via UART
void os_write(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

// Function to receive a line of input via UART
void os_read(char *buffer, int max_length) {
    int i = 0;
    char c;
    while (i < max_length - 1) { // Leave space for null terminator
        c = uart_getc();
        if (c == '\n' || c == '\r') {
            uart_putc('\n'); // Echo newline
            break;
        }
        uart_putc(c); // Echo character
        buffer[i++] = c;
    }
    buffer[i] = '\0'; // Null terminate the string
}

// Helper function to print an unsigned integer
void os_write_num(unsigned int num) {
    char buf[10];
    int i = 0;
    if (num == 0) {
        uart_putc('0');
        uart_putc('\n');
        return;
    }
    do {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    } while (num > 0 && i < 10);
    while (i > 0) {
        uart_putc(buf[--i]);
    }
    uart_putc('\n');
}

// Convert a string representing an integer to an actual integer
int alpha2int(const char *s) {
    int num = 0;
    int sign = 1;
    int i = 0;

    // Handle optional sign
    if (s[i] == '-') {
        sign = -1;
        i++;
    }

    for (; s[i] >= '0' && s[i] <= '9'; i++) {
        num = num * 10 + (s[i] - '0');
    }

    return sign * num;
}

// Convert a string representing a float to an actual float
float alpha2float(const char *s) {
    float num = 0;
    float sign = 1;
    int i = 0;
    int f = 10;

    // Handle optional sign
    if (s[i] == '-') {
        sign = -1;
        i++;
    }

    for (; s[i] >= '0' && s[i] <= '9'; i++) {
        num = num * 10 + (s[i] - '0');
    }

    if (s[i] != '.') return sign * num;

    i++;
    for (; s[i] >= '0' && s[i] <= '9'; i++) {
        num = num + (float)(s[i] - '0') / (float)f;
        f *= 10;
    }

    return sign * num;
}

// Convert an integer to a string representation of that integer
char * int2alpha(int num, char *buffer) {
    int i = 0;
    int is_negative = 0;

    if (num == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return buffer;
    }

    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    while (num > 0 && i < 14) { // Reserve space for sign and null terminator
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    // Reverse the string
    int start = 0, end = i - 1;
    char temp;
    while (start < end) {
        temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }

    return buffer;
}

// Convert a float to a string representation of that float
char * float2alpha(float num, char *buffer) {
    int i = 0;
    int is_negative = 0;
    int f_value;
    float decimal_part;
    int decimal_digits = 7;

    // Handle zero
    if (num == 0.0f) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return buffer;
    }

    // Handle negative
    if (num < 0.0f) {
        is_negative = 1;
        num = -num;
    }

    // Extract integer part
    f_value = (int)num;

    // Extract decimal part
    decimal_part = num - (float)f_value;

    // Add sign
    if (is_negative) {
        buffer[i++] = '-';
    }

    // Convert integer part
    char int_buffer[INT_TO_STRING_BUFFER_SIZE];
    char *int_str = int2alpha(f_value, int_buffer);
    while (*int_str) {
        buffer[i++] = *int_str++;
    }

    // Add decimal point
    buffer[i++] = '.';

    // Convert decimal part digit by digit
    for (int d = 0; d < decimal_digits; d++) {
        decimal_part *= 10.0f;
        int digit = (int)decimal_part;
        buffer[i++] = '0' + digit;
        decimal_part -= (float)digit;
    }

    buffer[i] = '\0';
    return buffer;
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

void read(char *buffer, const int buffer_size, const char *s, ...) {
    va_list args;
    va_start(args, s);
    vprint(s, args);
    va_end(args);
    os_read(buffer, buffer_size);
}