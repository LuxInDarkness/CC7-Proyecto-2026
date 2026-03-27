#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "pcb.h"
#include "scheduler.h"

void context_switch(StackFrame * frame, int quantums, int is_irq, int original_sp);
int swi_c_handler(StackFrame * frame, int original_sp);

#endif // INTERRUPTS_H