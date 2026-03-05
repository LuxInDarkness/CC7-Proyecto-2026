#include "os.h"

typedef struct REG_ACCESS
{
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
};

#define ACCESS ((struct REG_ACCESS *)0)

void os_init_regs(struct REG_ACCESS *regs) {

#if defined(PLATFORM_VERSATILEPB)
    regs->UART0_BASE = 0x101F1000;
    regs->UART_THR = regs->UART0_BASE + 0x00; // Equivalent to UART_DR
    regs->UART_LSR = regs->UART0_BASE + 0x18; // Equivalent to UART_FR
#else
    regs->UART0_BASE = 0x44E09000;
    regs->UART_THR = regs->UART0_BASE + 0x00;
    regs->UART_LSR = regs->UART0_BASE + 0x14;
    regs->DMTIMER2_BASE = 0x48040000;
    regs->TCLR = regs->DMTIMER2_BASE + 0x38;
    regs->TCRR = regs->DMTIMER2_BASE + 0x3C;
    regs->TISR = regs->DMTIMER2_BASE + 0x28;
    regs->TIER = regs->DMTIMER2_BASE + 0x2C;
    regs->TLDR = regs->DMTIMER2_BASE + 0x40;
    regs->INTCPS_BASE = 0x48200000;
    regs->INTC_MIR_CLEAR2 = regs->INTCPS_BASE + 0xC8;
    regs->INTC_CONTROL = regs->INTCPS_BASE + 0x48;
    regs->INTC_ILR68 = regs->INTCPS_BASE + 0x110;
    regs->CM_PER_BASE = 0x44E00000;
    regs->CM_PER_TIMER2_CLKCTRL = regs->CM_PER_BASE + 0x80;
#endif

    // Initialize UART Line Status Register bits
    regs->UART_LSR_THRE = 0x20;
    regs->UART_LSR_RXFE = 0x10;

}

typedef struct PCB {
    // Process Control Block structure (can be expanded as needed)
    int pid; // Process ID
    // Add more fields as necessary (e.g., state, registers, etc.)
} PCB;

typedef struct TCB {
    // Thread Control Block structure (can be expanded as needed)
    int tid; // Thread ID
    // Add more fields as necessary (e.g., state, registers, etc.)
} TCB;

// ============================================================================
// UART Functions
// ============================================================================

void uart_putc(char c) {
    // Wait until Transmit Holding Register is empty
    while ((GET32(ACCESS->UART_LSR) & ACCESS->UART_LSR_THRE) == 0);
    PUT32(ACCESS->UART_THR, c);
}

char uart_getc(void) {
    // Wait until data is available
    while ((GET32(ACCESS->UART_LSR) & ACCESS->UART_LSR_RXFE) != 0);
    return (char)(GET32(ACCESS->UART_THR) & 0xFF);
}

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
void uart_putnum(unsigned int num) {
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