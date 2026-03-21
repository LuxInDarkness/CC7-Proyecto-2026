#include "os.h"

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
    create_process(3, "P3", (process_entry_t)P3_ENTRY_ADDR, P3_STACK_TOP_ADDR);
}

int main() {
    os_init_regs();

    init_queue(&process_queue);
    print("Inicializada cola de procesos\n");

    initialize_processes();
    print("Procesos creados: P1, P2 y P3\n");

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
    }
}