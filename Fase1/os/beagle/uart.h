#ifndef UART_H
#define UART_H

#define UART0_BASE 0x44E09000
#define UART_THR (UART0_BASE + 0x00)
#define UART_LSR (UART0_BASE + 0x14)
#define UART_LSR_THRE 0x20
#define UART_LSR_RXFE 0x10

// Function prototypes for UART operations
void uart_putc(char c);
char uart_getc(void);

// Low-level memory access functions (implemented in root.s)
void PUT32(unsigned int addr, unsigned int value);
unsigned int GET32(unsigned int addr);

#endif // UART_H