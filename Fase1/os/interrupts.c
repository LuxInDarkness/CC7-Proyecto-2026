#include "interrupts.h"
#include "os_io.h"

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
    frame->r[0] = -1;  // return value of -1 indicates unknown syscall
    return original_sp;
}

// Returns 1 if the range [addr, addr+len) is within the process's mapped region
// Each process has a 64KB region starting at its entry address
static int is_valid_user_range(unsigned int addr, unsigned int len) {
    if (ACTIVE_PROCESS == 0) return 0;
    if (len == 0) return 1;

    unsigned int proc_base = (unsigned int)ACTIVE_PROCESS->entry;
    unsigned int proc_top  = proc_base + 0x10000u;  // 64KB region per linker scripts

    // Check for integer overflow in addr+len
    if (addr + len < addr) return 0;  // overflow
    if (addr < proc_base) return 0;   // starts before process region
    if (addr + len > proc_top) return 0; // ends after process region

    return 1;
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
    frame->r[0] = 0;  // return value of yield is always 0
    frame->lr = ACTIVE_PROCESS->pc;

    print("Yielding to process %s\n", ACTIVE_PROCESS->name);
    
    return ACTIVE_PROCESS->sp;
}

// SYS_WRITE — write buffer to UART, return to same process
// frame->r[1] = fd (always UART)
// frame->r[2] = buf pointer (address in process memory)
// frame->r[3] = size in bytes
static void syscall_write(StackFrame *frame, int original_sp) {
    const int fd = frame->r[1];
    unsigned int buf_addr = (unsigned int)frame->r[2];
    unsigned int size = (unsigned int)frame->r[3];

    if (fd != 1) {
        // Invalid fd — write nothing and return 0 bytes written
        print("Invalid fd %d in syscall write\n", fd);
        frame->r[0] = -2; // Invalid descriptor
        return;
    }

    // Cap size to a sane maximum (prevent runaway writes)
    if (size > 4096u) size = 4096u;

    // Validate user pointer range — must be within process's 64KB region
    if (!is_valid_user_range(buf_addr, size)) {
        frame->r[0] = -3;   // invalid user pointer
        return;
    }

    // Safe to dereference — pointer is validated
    const char *buf = (const char *)buf_addr;
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

    extern void hang(void);
    frame->lr = (int)hang;   // safe fallback — halts without crashing
    return os_idle_sp;
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

