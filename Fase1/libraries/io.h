#ifndef IO_H
#define IO_H

extern void uart_putc(char c);
extern char uart_getc(void);
void os_write(const char *s);
void os_read(char *buffer, int max_length);
void os_write_num(unsigned int num);
int alpha2int(const char *s);
char *int2alpha(int num, char *buffer);
char *float2alpha(float num, char *buffer);
float alpha2float(const char *s);
void print(const char *s, ...);
void read(char *buffer, const int buffer_size, const char *s, ...);

#endif // IO_H