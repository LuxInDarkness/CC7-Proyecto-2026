#include "os.h"
#include "../../libraries/io.h"

static REG_ACCESS access_block;
static REG_ACCESS *ACCESS = &access_block;

#define P1_ENTRY_ADDR 0x00100000u
#define P2_ENTRY_ADDR 0x00200000u

#define P1_STACK_TOP_ADDR 0x00110000u
#define P2_STACK_TOP_ADDR 0x00210000u

static ProcessQueue process_queue;
static PCB *active_process = 0;

static void move_process(PCB * pcb, int new_state) {
    PCB removed = remove_process(&process_queue, pcb);
    if (removed.pid == -1) return;
    set_process_state(&removed, new_state);
    enqueue_process(&process_queue, &removed);
}

static PCB * create_process(int pid, const char *name, process_entry_t entry, unsigned int stack_top) {
    if (process_queue.ready_index >= MAX_PROCESSES) {
        print("Error: no hay espacio en ready pool\n");
        return 0;
    }

    PCB *pcb = &process_queue.ready_pool[process_queue.ready_index];
    initialize_pcb(pcb, pid);
    configure_process(pcb, name, entry);
    setup_initial_process_stack(pcb, stack_top);
    process_queue.ready_index++;

    return pcb;
}

static void initialize_processes(void) {
    create_process(1, "P1", (process_entry_t)P1_ENTRY_ADDR, P1_STACK_TOP_ADDR);
    create_process(2, "P2", (process_entry_t)P2_ENTRY_ADDR, P2_STACK_TOP_ADDR);
}

static void activate_process(PCB *pcb) {
    if (active_process != 0 && active_process != pcb && active_process->state == RUNNING) {
        move_process(active_process, READY);
        active_process = 0;
    }

    // Promote incoming process to running
    move_process(pcb, RUNNING);
    pcb = 0;

    // Refresh active_process into its new location in running pool
    active_process = &process_queue.running_pool[process_queue.running_index - 1];

    print("Proceso %s activado\n", active_process->name);
}

static void run_active_process(void) {
    if (active_process != 0 && active_process->entry != 0) {
        active_process->entry(active_process);
    }
}

int main() {
    os_init_regs();

    init_queue(&process_queue);
    print("Inicializada cola de procesos\n");

    initialize_processes();
    print("Procesos creados: P1 y P2\n");

    unsigned int cd_value = 100000;
    timer_init(cd_value);

    enable_irq();
    print("\n -- SO iniciado -- \n");

    while (1) {
        print("OS idling...............................\n");
        sleep(0.2);
    }

    return 0;
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

void context_switch(IRQFrame * frame) {
    if (process_queue.ready_index == 0) return;

    // Save current process — read directly from the IRQ stack frame
    if (active_process != 0) {
        save_process_state(active_process, frame);      // save into pool entry directly
        move_process(active_process, READY);            // then move to ready with state intact
        active_process = 0;
    }

    // Restore next process — overwrite the IRQ stack frame
    if (process_queue.ready_index > 0) {
        PCB *next_ready = &process_queue.ready_pool[0];
        move_process(next_ready, RUNNING);
        next_ready = 0;                  // stale after move

        // Refresh pointer into running pool
        active_process = &process_queue.running_pool[process_queue.running_index - 1];
        restore_process_state(active_process, frame);

        print("--------------Context switched to process %s---------------------------\n", active_process->name);
    }
}

void timer_irq_handler(IRQFrame * frame) {
    // Clear the timer interrupt
    *(ACCESS->TIMER0_INTCLR) = 1;

    context_switch(frame);

    // Signal end of interrupt
    *(ACCESS->VIC_VECTADDR) = 0;
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