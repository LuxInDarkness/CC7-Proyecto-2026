#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "scheduler.h"

extern int os_idle_sp;

enum SyscallID {
    SYS_YIELD = 0,
    SYS_EXIT = 1,
    SYS_WRITE = 2
};

void context_switch(StackFrame * frame, int quantums, int is_irq, int original_sp);
int swi_c_handler(StackFrame * frame, int original_sp);
static int syscall_yield(StackFrame *frame, int original_sp);
static int syscall_exit(StackFrame *frame, int original_sp);
static void syscall_write(StackFrame *frame, int original_sp);

#endif // INTERRUPTS_H