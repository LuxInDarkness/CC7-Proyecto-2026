#include "interrupts.h"
#include "os_io.h"

int swi_c_handler(StackFrame *frame, int original_sp) {
    int syscall_id = frame->r[0];  // r0 at time of svc instruction
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
        // Unknown syscall — return to same process unchanged
        frame->r[0] = -1;  // return value of -1 indicates unknown syscall
        result_sp = original_sp;
    }

    // === TRACE: KERNEL_TO_USER syscall_return ===
    if (ACTIVE_PROCESS != 0) {
        print("MODE_SWITCH KERNEL_TO_USER pid=%d reason=syscall_return id=%d rc=%d\n",
              ACTIVE_PROCESS->pid, syscall_id, frame->r[0]);
    }

    return result_sp;
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
    static int initial_launch_traced = 0;

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
        save_process_state(ACTIVE_PROCESS, frame, is_irq, original_sp);
        move_process(ACTIVE_PROCESS, READY);
        ACTIVE_PROCESS = 0;
    }

    // Restore next process
    PCB *next_ready = &QUEUE->ready_pool[0];
    move_process(next_ready, RUNNING);
    next_ready = 0;

    ACTIVE_PROCESS = &QUEUE->running_pool[QUEUE->running_index - 1];
    restore_process_state(ACTIVE_PROCESS, frame);

    // === TRACE: KERNEL_TO_USER (initial_launch or dispatch) ===
    if (is_irq && ACTIVE_PROCESS != 0) {
        if (!initial_launch_traced) {
            initial_launch_traced = 1;
            print("MODE_SWITCH KERNEL_TO_USER pid=%d reason=initial_launch\n",
                  ACTIVE_PROCESS->pid);
        } else {
            print("MODE_SWITCH KERNEL_TO_USER pid=%d reason=dispatch\n",
                  ACTIVE_PROCESS->pid);
        }
    }
}

