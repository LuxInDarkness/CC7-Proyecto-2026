#include "interrupts.h"
#include "os_io.h"

int swi_c_handler(StackFrame *frame, int original_sp) {
    // Validate caller originated from USR mode
    int spsr = read_spsr_svc();
    int caller_mode = spsr & 0x1F;
    if (caller_mode != 0x10) {
        frame->r[0] = -1;       // reject: not a user syscall
        return original_sp;     // return to same context unchanged
    }

    int syscall_id = frame->r[0];  // r0 at time of svc instruction
    int arg1        = frame->r[1];  // r1 — exit code for SYS_EXIT
    int result_sp;

    // === TRACE: USER_TO_KERNEL syscall ===
    if (ACTIVE_PROCESS != 0) {
        print("MODE_SWITCH USER_TO_KERNEL pid=%d reason=syscall id=%d\n",
              ACTIVE_PROCESS->pid, syscall_id);
    }

    if (syscall_id == SYS_YIELD) {
        result_sp = syscall_yield(frame, original_sp);
    } else if (syscall_id == SYS_EXIT) {
        result_sp = syscall_exit(frame, original_sp);
    } else if (syscall_id == SYS_WRITE) {
        syscall_write(frame, original_sp);
        result_sp = original_sp;  // no context switch — return to same process
    } else {
        // Unknown syscall — return to same process with error
        frame->r[0] = -1;  // -1: invalid syscall ID
        result_sp = original_sp;
    }

    // === TRACE: KERNEL_TO_USER syscall_return ===
    if (ACTIVE_PROCESS != 0) {
        int rc = (syscall_id == SYS_EXIT) ? arg1 : frame->r[0];
        print("MODE_SWITCH KERNEL_TO_USER pid=%d reason=syscall_return id=%d rc=%d\n",
              ACTIVE_PROCESS->pid, syscall_id, rc);
    }

    return result_sp;
}

// Returns 1 if the range [addr, addr+len) is within the process's mapped region.
// Each process has a 64KB region starting at its base address.
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
        save_process_state(ACTIVE_PROCESS, frame);
        move_process(ACTIVE_PROCESS, READY);
        ACTIVE_PROCESS = 0;
    }

    PCB *next_ready = &QUEUE->ready_pool[0];
    move_process(next_ready, RUNNING);
    next_ready = 0;

    ACTIVE_PROCESS = &QUEUE->running_pool[QUEUE->running_index - 1];

    restore_process_state(ACTIVE_PROCESS, frame);
    frame->r[0] = 0;  // yield return value is always 0

    print("Yielding to process %s\n", ACTIVE_PROCESS->name);

    return ACTIVE_PROCESS->sp;
}

// SYS_WRITE — write buffer to UART, return to same process
// frame->r[1] = fd (must be 1 = UART)
// frame->r[2] = buf pointer (address in process memory)
// frame->r[3] = size in bytes
static void syscall_write(StackFrame *frame, int original_sp) {
    const int fd = frame->r[1];
    unsigned int buf_addr = (unsigned int)frame->r[2];
    unsigned int size = (unsigned int)frame->r[3];

    if (fd != 1) {
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
    frame->r[0] = (int)size;
}

// SYS_EXIT — terminate the calling process and switch to next.
// Never returns to the caller — the exit code is saved in arg1 before
// this function runs, and is used for the trace in swi_c_handler.
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

        restore_process_state(ACTIVE_PROCESS, frame);

        return ACTIVE_PROCESS->sp;
    }

    // No runnable task — halt safely
    extern void hang(void);
    frame->lr = (int)hang;
    return os_idle_sp;
}

void context_switch(StackFrame *frame, int quantums, int is_irq) {
    // === TRACE: USER_TO_KERNEL timer_irq ===
    if (is_irq && ACTIVE_PROCESS != 0) {
        print("MODE_SWITCH USER_TO_KERNEL pid=%d reason=timer_irq\n",
              ACTIVE_PROCESS->pid);
    }

    if (QUEUE->ready_index == 0) {
        if (is_irq && ACTIVE_PROCESS != 0) {
            print("MODE_SWITCH KERNEL_TO_USER pid=%d reason=dispatch\n",
                  ACTIVE_PROCESS->pid);
        }
        return;
    }

    ACTIVE_PROCESS->curr_quantums += quantums;
    if (ACTIVE_PROCESS->curr_quantums < ACTIVE_PROCESS->max_quantums) {
        print("%s has been active %d quantums, with a max %d quantums\n",
              ACTIVE_PROCESS->name, ACTIVE_PROCESS->curr_quantums,
              ACTIVE_PROCESS->max_quantums);
        print("Not time to change yet........................................................\n");
        if (is_irq && ACTIVE_PROCESS != 0) {
            print("MODE_SWITCH KERNEL_TO_USER pid=%d reason=dispatch\n",
                  ACTIVE_PROCESS->pid);
        }
        return;
    }

    ACTIVE_PROCESS->curr_quantums = 0;
    print("Process %s, time to change...............................................\n",
          ACTIVE_PROCESS->name);

    // Save current process
    if (ACTIVE_PROCESS != 0) {
        save_process_state(ACTIVE_PROCESS, frame);
        move_process(ACTIVE_PROCESS, READY);
        ACTIVE_PROCESS = 0;
    }

    // Restore next process
    PCB *next_ready = &QUEUE->ready_pool[0];
    move_process(next_ready, RUNNING);
    next_ready = 0;

    ACTIVE_PROCESS = &QUEUE->running_pool[QUEUE->running_index - 1];
    restore_process_state(ACTIVE_PROCESS, frame);

    // === TRACE: KERNEL_TO_USER dispatch ===
    if (is_irq && ACTIVE_PROCESS != 0) {
        print("MODE_SWITCH KERNEL_TO_USER pid=%d reason=dispatch\n",
              ACTIVE_PROCESS->pid);
    }
}

