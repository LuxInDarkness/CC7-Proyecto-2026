#ifndef OS_H
#define OS_H

#include "../pcb.h"

typedef struct REG_ACCESS
{
    volatile unsigned int UART0_BASE; // Base address for UART0
    volatile unsigned int UART_DR;        // Data Register
    volatile unsigned int UART_FR;        // Flag Register
    volatile unsigned int UART_FR_TXFF;     // Transmit FIFO Full
    volatile unsigned int UART_FR_RXFE;     // Receive FIFO Empty
    volatile unsigned int TISR;           // Timer Interrupt Status Register
} REG_ACCESS;

void os_init_regs(void);
static void create_process(PCB *pcb, int pid, const char *name, process_entry_t entry, unsigned int stack_top);
static void initialize_processes(void);
static PCB *find_process_by_command(char cmd);
static void activate_process(PCB *pcb);
static void run_active_process(void);
void timer_irq_handler();

#endif // OS_H
