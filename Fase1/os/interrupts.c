#include "interrupts.h"
#include "../libraries/io.h"

int swi_c_handler(StackFrame * frame, int original_sp) {
    if (QUEUE->ready_index == 0) return original_sp;

    ACTIVE_PROCESS->curr_quantums = ACTIVE_PROCESS->max_quantums;

    if (ACTIVE_PROCESS != 0) {
        save_process_state(ACTIVE_PROCESS, frame, 0, original_sp);
        move_process(ACTIVE_PROCESS, READY);
        ACTIVE_PROCESS = 0;
    }

    PCB *next_ready = &QUEUE->ready_pool[0];
    move_process(next_ready, RUNNING);
    next_ready = 0;

    ACTIVE_PROCESS = &QUEUE->running_pool[QUEUE->running_index - 1];
    // Write registers into frame but DO NOT touch sp yet
    // sp switch happens in assembly after ldmfd
    int i;
    for (i = 0; i < 13; i++)
        frame->r[i] = ACTIVE_PROCESS->registers[i];
    frame->lr = ACTIVE_PROCESS->pc;
    return ACTIVE_PROCESS->sp;  // return next SP to assembly
}

void context_switch(StackFrame * frame, int quantums, int is_irq, int original_sp) {
    if (QUEUE->ready_index == 0) return;

    ACTIVE_PROCESS->curr_quantums += quantums;
    if (ACTIVE_PROCESS->curr_quantums < ACTIVE_PROCESS->max_quantums) {
        print("%s has been active %d quantums, with a max %d quantums\n", ACTIVE_PROCESS->name, ACTIVE_PROCESS->curr_quantums, ACTIVE_PROCESS->max_quantums);
        print("Not time to change yet........................................................\n");
        return;
    }

    ACTIVE_PROCESS->curr_quantums = 0;
    print("Process %s, time to change...............................................\n", ACTIVE_PROCESS->name);

    // Save current process — read directly from the IRQ stack frame
    if (ACTIVE_PROCESS != 0) {
        save_process_state(ACTIVE_PROCESS, frame, is_irq, original_sp);      // save into pool entry directly
        move_process(ACTIVE_PROCESS, READY);            // then move to ready with state intact
        ACTIVE_PROCESS = 0;
    }

    // Restore next process — overwrite the IRQ stack frame
    PCB *next_ready = &QUEUE->ready_pool[0];
    move_process(next_ready, RUNNING);
    next_ready = 0;                  // stale after move

    // Refresh pointer into running pool
    ACTIVE_PROCESS = &QUEUE->running_pool[QUEUE->running_index - 1];
    restore_process_state(ACTIVE_PROCESS, frame);
}
