#include "os.h"
#include "../pcb.h"
#include "../../libraries/io.h"

static REG_ACCESS access_block;
static REG_ACCESS *ACCESS = &access_block;

#define P1_ENTRY_ADDR 0x00100000u
#define P2_ENTRY_ADDR 0x00200000u

#define P1_STACK_TOP_ADDR 0x00110000u
#define P2_STACK_TOP_ADDR 0x00210000u

#define PROCESS_COUNT 2

static PCB process_table[PROCESS_COUNT];
static PCB *active_process = 0;

static void create_process(PCB *pcb, int pid, const char *name, process_entry_t entry, unsigned int stack_top) {
    initialize_pcb(pcb, pid);
    configure_process(pcb, name, entry);
    setup_initial_process_stack(pcb, stack_top);
}

static void initialize_processes(void) {
    create_process(&process_table[0], 1, "P1", (process_entry_t)P1_ENTRY_ADDR, P1_STACK_TOP_ADDR);
    create_process(&process_table[1], 2, "P2", (process_entry_t)P2_ENTRY_ADDR, P2_STACK_TOP_ADDR);
}

static PCB *find_process_by_command(char cmd) {
    if (cmd == '1') {
        return &process_table[0];
    }

    if (cmd == '2') {
        return &process_table[1];
    }

    return 0;
}

static void activate_process(PCB *pcb) {
    if (active_process != 0 && active_process != pcb && active_process->state == RUNNING) {
        set_process_state(active_process, READY);
    }

    active_process = pcb;
    set_process_state(active_process, RUNNING);
    print("Proceso %s activado dentro del SO\n", active_process->name);
}

static void run_active_process(void) {
    if (active_process != 0 && active_process->entry != 0) {
        active_process->entry(active_process);
    }
}

int main() {
    os_init_regs();
    initialize_processes();

    print("\n -- SO iniciado -- \n");
    print("Procesos creados: P1 y P2\n");
    print("Presionar 1 para ejecutar P1 o 2 para ejecutar P2\n");

    while (1) {
        if (active_process == 0) {
            PCB *selected_process = find_process_by_command(uart_getc());

            if (selected_process == 0) {
                print("Elegir 1 o 2\n");
                continue;
            }

            activate_process(selected_process);
        }

        run_active_process();
    }

    return 0;
}

void os_init_regs(void) {
    
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