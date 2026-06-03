#include "io.h"
#include "my_stdarg.h"

#define INT_TO_STRING_BUFFER_SIZE 32
#define FLOAT_TO_STRING_BUFFER_SIZE 32
#define PRINT_BUFFER_SIZE 256

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
