#ifndef OS_H
#define OS_H

typedef struct REG_ACCESS {
    volatile unsigned int UART0_BASE; // Base address for UART0
    volatile unsigned int UART_THR; // Transmit Holding Register (offset 0x00), equivalent to UART_DR for VersatilePB
    volatile unsigned int UART_LSR; // Line Status Register (offset 0x14), equivalent to UART_FR for VersatilePB
    volatile unsigned int UART_LSR_THRE; // Transmit Holding Register Empty bit (0x20)
    volatile unsigned int UART_LSR_RXFE; // Receiver FIFO Empty bit (0x10)
    volatile unsigned int DMTIMER2_BASE; // Base address for DMTIMER2
    volatile unsigned int TCLR; // Timer Control Register (offset 0x38)
    volatile unsigned int TCRR; // Timer Counter Register (offset 0x3C)
    volatile unsigned int TISR; // Timer Interrupt Status Register (offset 0x28)
    volatile unsigned int TIER; // Timer Interrupt Enable Register (offset 0x2C)
    volatile unsigned int TLDR; // Timer Load Register (offset 0x40)
    volatile unsigned int INTCPS_BASE; // Base address for Interrupt Controller (INTCPS)
    volatile unsigned int INTC_MIR_CLEAR2; // Interrupt Mask Clear Register 2 (offset 0xC8)
    volatile unsigned int INTC_CONTROL; // Interrupt Controller Control (offset 0x48)
    volatile unsigned int INTC_ILR68; // Interrupt Line Register 68 (offset 0x110)
    volatile unsigned int CM_PER_BASE; // Base address for Clock Manager
    volatile unsigned int CM_PER_TIMER2_CLKCTRL; // Timer2 Clock Control (offset 0x80)
} REG_ACCESS;

// Function to initialize the REG_ACCESS structure with the correct addresses
void os_init_regs(int address);

#endif // OS_H
