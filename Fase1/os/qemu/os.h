#ifndef OS_H
#define OS_H

typedef struct REG_ACCESS
{
    volatile unsigned int UART0_BASE; // Base address for UART0
    volatile unsigned int UART_DR;        // Data Register
    volatile unsigned int UART_FR;        // Flag Register
    volatile unsigned int UART_FR_TXFF;     // Transmit FIFO Full
    volatile unsigned int UART_FR_RXFE;     // Receive FIFO Empty
    volatile unsigned int TISR;           // Timer Interrupt Status Register
} REG_ACCESS;

void os_init_regs(int address);

#endif // OS_H
