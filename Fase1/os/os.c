#include "os.h"

static void initialize_processes(void) {
    create_process(0, "OS", (process_entry_t)OS_ENTRY_ADDR, OS_STACK_TOP_ADDR, 1);
    create_process(1, "P1", (process_entry_t)P1_ENTRY_ADDR, P1_STACK_TOP_ADDR, 1);
    create_process(2, "P2", (process_entry_t)P2_ENTRY_ADDR, P2_STACK_TOP_ADDR, 2);
    create_process(3, "P3", (process_entry_t)P3_ENTRY_ADDR, P3_STACK_TOP_ADDR, 1);
}

int main() {
    watchdog_disable();
    os_init_regs();
    intc_init();

    init_queue(QUEUE);
    print("Inicializada cola de procesos\n");

    initialize_processes();
    print("Procesos creados: P1, P2, P3 y OS\n");

    PCB *next_ready = &QUEUE->ready_pool[0];
    move_process(next_ready, RUNNING);
    ACTIVE_PROCESS = &QUEUE->running_pool[QUEUE->running_index - 1];
    print("Proceso %s activado...", ACTIVE_PROCESS->name);

    timer_init(calculate_timer_cd(500));

    enable_irq();
    print("\n -- SO iniciado -- \n");

    while (1) {
        print("OS idling...............................\n");
        sleep(0.2);
    }

    return 0;
}
