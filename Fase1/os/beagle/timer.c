#include "timer.h"

static REG_ACCESS access_block;
static REG_ACCESS *ACCESS = &access_block;

void intc_init(void) {
    PUT32(ACCESS->INTCPS_BASE + 0x10, 0x2);                      // soft reset INTC
    while (!(GET32(ACCESS->INTCPS_BASE + 0x14) & 0x1));          // wait reset done
    PUT32(ACCESS->INTCPS_BASE + 0x50, 0x0);                      // disable clock gating
    PUT32(ACCESS->INTCPS_BASE + 0x68, 0xFF);                     // disable threshold
}

void timer_init(unsigned int load_value) {
    /* --- Select 24MHz clock for Timer2 --------------------------------- */
    PUT32(ACCESS->CLKSEL_TIMER2_CLK, 0x1);

    /* --- Enable Timer2 clock -------------------------------------------- */
    PUT32(ACCESS->CM_PER_TIMER2_CLKCTRL, 0x2);
    while ((GET32(ACCESS->CM_PER_TIMER2_CLKCTRL) & 0x30000) != 0); // wait active

    /* --- Configure Timer2 IRQ in INTC ----------------------------------- */
    PUT32(ACCESS->INTC_MIR_CLEAR2, 1 << 4);  // unmask IRQ68 (Timer2) in bank 2
    PUT32(ACCESS->INTC_ILR68, 0x0);           // priority 0, route as IRQ not FIQ

    /* --- Configure DMTimer2 --------------------------------------------- */
    PUT32(ACCESS->TCLR, 0x0);                 // disable timer before configuring
    PUT32(ACCESS->TISR, 0x7);                 // clear all pending interrupt flags
    PUT32(ACCESS->TLDR, load_value);          // set reload value
    PUT32(ACCESS->TCRR, load_value);          // set current counter to same value
    PUT32(ACCESS->TIER, 0x2);                 // enable overflow interrupt

    /* --- Start timer and agree first IRQ -------------------------------- */
    PUT32(ACCESS->TCLR, 0x3);                 // enable timer, auto-reload mode
    PUT32(ACCESS->INTC_CONTROL, 0x1);         // NewIRQAgree — start IRQ logic
}

void timer_irq_handler(IRQFrame * frame) {
    // Clear the timer interrupt
    PUT32(ACCESS->TISR, 0x2);
    context_switch(frame);
    PUT32(ACCESS->INTC_CONTROL, 0x1);
}

void os_init_regs(void) {
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
    ACCESS->CLKSEL_TIMER2_CLK = ACCESS->CM_PER_BASE + 0x508;
    ACCESS->UART_LSR_THRE = 0x20;
    ACCESS->UART_LSR_RXFE = 0x10;
}