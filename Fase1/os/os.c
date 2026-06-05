#include "os.h"

int os_idle_sp = 0;

/* Assembly: enters USR mode with the given SP, PC, SPSR. Never returns. */
extern void enter_first_task(int sp, int pc, int spsr) __attribute__((noreturn));

static void initialize_processes(void) {
    create_process(1, "P1", (process_entry_t)P1_ENTRY_ADDR, P1_STACK_TOP_ADDR, 1);
    create_process(2, "P2", (process_entry_t)P2_ENTRY_ADDR, P2_STACK_TOP_ADDR, 2);
    create_process(3, "P3", (process_entry_t)P3_ENTRY_ADDR, P3_STACK_TOP_ADDR, 1);
}

int main() {
    // Capture OS stack pointer before doing anything else
    __asm__ volatile ("mov %0, sp" : "=r"(os_idle_sp));

    watchdog_disable();
    os_init_regs();
    intc_init();

    init_queue(QUEUE);
    print("Inicializada cola de procesos\n");

    initialize_processes();
    print("Procesos creados: P1, P2, P3\n");

    // Activate the first user process (P1)
    PCB *first = &QUEUE->ready_pool[0];
    move_process(first, RUNNING);
    first = 0;
    ACTIVE_PROCESS = &QUEUE->running_pool[QUEUE->running_index - 1];
    print("Proceso %s activado...\n", ACTIVE_PROCESS->name);

    // Start timer for preemptive scheduling
    timer_init(calculate_timer_cd(500));

    // --- Explicit initial launch (Section 3.4) ---
    print("MODE_SWITCH KERNEL_TO_USER pid=%d reason=initial_launch\n",
          ACTIVE_PROCESS->pid);
    print(" -- SO iniciado -- \n");

    // Write user SP now so it's ready for the IRQ handler on first preemption
    write_usr_sp(ACTIVE_PROCESS->sp);

    // Enter USR mode — never returns
    enter_first_task(ACTIVE_PROCESS->sp,
                     ACTIVE_PROCESS->pc,
                     ACTIVE_PROCESS->spsr);

    // Unreachable
    while (1) {}
    return 0;
}
