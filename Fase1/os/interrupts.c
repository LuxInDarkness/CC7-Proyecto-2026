#include "interrupts.h"
#include "io.h"

int swi_c_handler(StackFrame *frame, int original_sp) {
    int syscall_id = frame->r[0];  // r0 at time of svc instruction

    if (syscall_id == SYS_YIELD) {
        return syscall_yield(frame, original_sp);
    } else if (syscall_id == SYS_EXIT) {
        return syscall_exit(frame, original_sp);
    } else if (syscall_id == SYS_WRITE) {
        syscall_write(frame, original_sp);
        return original_sp;  // no context switch — return to same process
    }

    // Unknown syscall — return to same process unchanged
    return original_sp;
}

// SYS_YIELD — save current process, switch to next ready process
static int syscall_yield(StackFrame *frame, int original_sp) {
    if (QUEUE->ready_index == 0) return original_sp;  // nothing to switch to

    if (ACTIVE_PROCESS != 0) {
        save_process_state(ACTIVE_PROCESS, frame, 0, original_sp);
        move_process(ACTIVE_PROCESS, READY);
        ACTIVE_PROCESS = 0;
    }

    PCB *next_ready = &QUEUE->ready_pool[0];
    move_process(next_ready, RUNNING);
    next_ready = 0;

    ACTIVE_PROCESS = &QUEUE->running_pool[QUEUE->running_index - 1];

    int i;
    for (i = 0; i < 13; i++)
        frame->r[i] = ACTIVE_PROCESS->registers[i];
    frame->lr = ACTIVE_PROCESS->pc;

    return ACTIVE_PROCESS->sp;
}

// SYS_WRITE — write buffer to UART, return to same process
// frame->r[1] = fd (ignored, always UART)
// frame->r[2] = buf pointer (address in process memory)
// frame->r[3] = size in bytes
static void syscall_write(StackFrame *frame, int original_sp) {
    const char *buf = (const char *)frame->r[2];
    unsigned int size = (unsigned int)frame->r[3];
    unsigned int i;

    for (i = 0; i < size; i++) {
        uart_putc(buf[i]);
    }

    // Write return value (bytes written) back into frame->r[0]
    // so the process receives it as the return value of sys_write()
    frame->r[0] = (int)size;
}

// SYS_EXIT — terminate the calling process and switch to next
// frame->r[1] = exit status code
static int syscall_exit(StackFrame *frame, int original_sp) {
    if (ACTIVE_PROCESS != 0) {
        ACTIVE_PROCESS->termination_status = frame->r[1];
        move_process(ACTIVE_PROCESS, TERMINATED);
        ACTIVE_PROCESS = 0;
    }

    // If there is a next process ready, switch to it
    if (QUEUE->ready_index > 0) {
        PCB *next_ready = &QUEUE->ready_pool[0];
        move_process(next_ready, RUNNING);
        next_ready = 0;

        ACTIVE_PROCESS = &QUEUE->running_pool[QUEUE->running_index - 1];

        int i;
        for (i = 0; i < 13; i++)
            frame->r[i] = ACTIVE_PROCESS->registers[i];
        frame->lr = ACTIVE_PROCESS->pc;

        return ACTIVE_PROCESS->sp;
    }

    // No ready process — return to original_sp (OS idle loop takes over)
    return original_sp;
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
