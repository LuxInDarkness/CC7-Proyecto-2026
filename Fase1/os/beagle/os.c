#include "os.h"

REG_ACCESS *ACCESS;

void os_init_regs(int address) {
    // Allocate memory for the REG_ACCESS structure
    ACCESS = (REG_ACCESS *)address;
    ACCESS->UART0_BASE = 0x44E09000;
    ACCESS->UART_THR = ACCESS->UART0_BASE + 0x00;
    ACCESS->UART_LSR = ACCESS->UART0_BASE + 0x14;
    ACCESS->DMTIMER2_BASE = 0x48040000;
    ACCESS->TCLR = ACCESS->DMTIMER2_BASE + 0x38;
    ACCESS->TCRR = ACCESS->DMTIMER2_BASE + 0x3C;
    ACCESS->TISR = ACCESS->DMTIMER2_BASE + 0x28;
    ACCESS->TIER = ACCESS->DMTIMER2_BASE + 0x2C;
    ACCESS->TLDR = ACCESS->DMTIMER2_BASE + 0x40;
    ACCESS->INTCPS_BASE = 0x48200000;
    ACCESS->INTC_MIR_CLEAR2 = ACCESS->INTCPS_BASE + 0xC8;
    ACCESS->INTC_CONTROL = ACCESS->INTCPS_BASE + 0x48;
    ACCESS->INTC_ILR68 = ACCESS->INTCPS_BASE + 0x110;
    ACCESS->CM_PER_BASE = 0x44E00000;
    ACCESS->CM_PER_TIMER2_CLKCTRL = ACCESS->CM_PER_BASE + 0x80;
    ACCESS->UART_LSR_THRE = 0x20;
    ACCESS->UART_LSR_RXFE = 0x10;
}

void timer_irq_handler() {
    // Clear the timer interrupt
    *(volatile unsigned int *)(ACCESS->TISR) = 0x1; // Clear the interrupt status
    // Additional code to handle the timer interrupt can be added here
}
