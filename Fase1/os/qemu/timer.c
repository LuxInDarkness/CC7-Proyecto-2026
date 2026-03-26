#include "timer.h"

static REG_ACCESS access_block;
static REG_ACCESS *ACCESS = &access_block;

void intc_init(void) {
    // No initialization needed in QEMU
}

void timer_init(unsigned int load_value) {
    /* --- SP804 Timer 0 setup -------------------------------------------- */
    // Disable timer
    *(ACCESS->TIMER0_CTRL) = 0;
    // Set the reload value
    *(ACCESS->TIMER0_LOAD) = load_value;
    /* 3. Configure and enable:
     *    PERIODIC — reloads from LOAD register on each underflow
     *    INTEN    — generates an interrupt on underflow
     *    32BIT    — use full 32-bit counter
     *    ENABLE   — start counting */
    *(ACCESS->TIMER0_CTRL) = TIMER_CTRL_PERIODIC |
                            TIMER_CTRL_INTEN |
                            TIMER_CTRL_32BIT |
                            TIMER_CTRL_ENABLE;

    /* --- PL190 VIC setup ------------------------------------------------- */
    // Make sure Timer 0 (bit 4) is classified as IRQ, not FIQ.
    *(ACCESS->VIC_INTSELECT) &= ~VIC_TIMER0_BIT;
    /* Unmask Timer 0 in the VIC — without this the interrupt signal
     * from SP804 never reaches the CPU */
    *(ACCESS->VIC_INTENABLE) = VIC_TIMER0_BIT;
}

void timer_irq_handler(IRQFrame * frame) {
    // Clear the timer interrupt
    *(ACCESS->TIMER0_INTCLR) = 1;

    context_switch(frame);

    // Signal end of interrupt
    *(ACCESS->VIC_VECTADDR) = 0;
}

int calculate_timer_cd(int milliseconds) {
    return CLOCK_FREQ / 1000 * milliseconds;
}

void os_init_regs(void) {
    
    ACCESS->UART0_BASE = 0x101F1000;
    ACCESS->UART_DR = 0x00;
    ACCESS->UART_FR = 0x18;
    ACCESS->UART_FR_TXFF = 0x20;
    ACCESS->UART_FR_RXFE = 0x10;
    ACCESS->TISR = 0x28;
    ACCESS->TIMERO_BASE = 0x101E2000;
    ACCESS->TIMER0_LOAD = ACCESS->TIMERO_BASE + 0x00;
    ACCESS->TIMER0_VALUE = ACCESS->TIMERO_BASE + 0x04;
    ACCESS->TIMER0_CTRL = ACCESS->TIMERO_BASE + 0x08;
    ACCESS->TIMER0_INTCLR = ACCESS->TIMERO_BASE + 0x0C;
    ACCESS->TIMER0_RIS = ACCESS->TIMERO_BASE + 0x10;
    ACCESS->TIMER0_MIS = ACCESS->TIMERO_BASE + 0x14;
    ACCESS->TIMER0_BGLOAD = ACCESS->TIMERO_BASE + 0x18;
    ACCESS->VIC_BASE = 0x10140000;
    ACCESS->VIC_IRQSTATUS = ACCESS->VIC_BASE + 0x000;
    ACCESS->VIC_FIQSTATUS = ACCESS->VIC_BASE + 0x004;
    ACCESS->VIC_RAWINTR = ACCESS->VIC_BASE + 0x008;
    ACCESS->VIC_INTSELECT = ACCESS->VIC_BASE + 0x00C;
    ACCESS->VIC_INTENABLE = ACCESS->VIC_BASE + 0x010;
    ACCESS->VIC_INTENCLR = ACCESS->VIC_BASE + 0x014;
    ACCESS->VIC_VECTADDR = ACCESS->VIC_BASE + 0x030;

}