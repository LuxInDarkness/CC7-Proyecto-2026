#include "os.h"

REG_ACCESS *ACCESS;

int main() {
    return 0;
}

void os_init_regs(int address) {

    ACCESS = (REG_ACCESS *)address;
    ACCESS->UART0_BASE = 0x101F1000;
    ACCESS->UART_DR = 0x00;
    ACCESS->UART_FR = 0x18;
    ACCESS->UART_FR_TXFF = 0x20;
    ACCESS->UART_FR_RXFE = 0x10;
    ACCESS->TISR = 0x28;

}

void timer_irq_handler() {
    // Clear the timer interrupt
    *(volatile unsigned int *)(ACCESS->TISR) = 0x1; // Clear the interrupt status
    // Additional code to handle the timer interrupt can be added here
}