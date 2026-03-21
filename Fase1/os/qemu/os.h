#ifndef OS_H
#define OS_H

#include "../pcb.h"
#include "../scheduler.h"
#include "../../libraries/time.h"

typedef struct REG_ACCESS
{
    volatile unsigned int UART0_BASE; // Base address for UART0
    volatile unsigned int UART_DR;        // Data Register
    volatile unsigned int UART_FR;        // Flag Register
    volatile unsigned int UART_FR_TXFF;     // Transmit FIFO Full
    volatile unsigned int UART_FR_RXFE;     // Receive FIFO Empty
    volatile unsigned int TISR;           // Timer Interrupt Status Register
    volatile unsigned int TIMERO_BASE;     // Base address for Timer0
    volatile unsigned int * TIMER0_LOAD;     // Timer0 Load Register
    volatile unsigned int * TIMER0_VALUE;    // Timer0 Current Value Register
    volatile unsigned int * TIMER0_CTRL;     // Timer0 Control Register
    volatile unsigned int * TIMER0_INTCLR;   // Timer0 Interrupt Clear Register
    volatile unsigned int * TIMER0_RIS;      // Timer0 Raw Interrupt Status Register
    volatile unsigned int * TIMER0_MIS;      // Timer0 Masked Interrupt Status Register
    volatile unsigned int * TIMER0_BGLOAD;   // Timer0 Background Load Register
    volatile unsigned int VIC_BASE;        // Base address for VIC
    volatile unsigned int * VIC_IRQSTATUS;   // VIC IRQ Status Register
    volatile unsigned int * VIC_FIQSTATUS;   // VIC FIQ Status Register
    volatile unsigned int * VIC_RAWINTR;     // VIC Raw Interrupt Status Register
    volatile unsigned int * VIC_INTSELECT;   // VIC Interrupt Select Register
    volatile unsigned int * VIC_INTENABLE;   // VIC Interrupt Enable Register
    volatile unsigned int * VIC_INTENCLR;    // VIC Interrupt Enable Clear Register
    volatile unsigned int * VIC_VECTADDR;    // VIC Vector Address Register
} REG_ACCESS;

/* TIMER0_CTRL bit fields */
#define TIMER_CTRL_ENABLE   (1 << 7)  /* 1 = timer enabled */
#define TIMER_CTRL_PERIODIC (1 << 6)  /* 1 = periodic mode (reloads from LOAD) */
#define TIMER_CTRL_INTEN    (1 << 5)  /* 1 = interrupt enabled */
#define TIMER_CTRL_32BIT    (1 << 1)  /* 1 = 32-bit counter, 0 = 16-bit */
#define TIMER_CTRL_ONESHOT  (1 << 0)  /* 1 = one-shot, 0 = wrapping */

/* Timer 0 is on VIC channel 4 (pic[4] in versatilepb.c) */
#define VIC_TIMER0_BIT      (1 << 4)

void enable_irq(void);
void timer_init(unsigned int load_value);
void os_init_regs(void);

#endif // OS_H
