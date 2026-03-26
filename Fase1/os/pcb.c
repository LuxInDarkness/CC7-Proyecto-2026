#include "pcb.h"

#define INITIAL_CONTEXT_WORDS 14u

void initialize_pcb(PCB *pcb, int pid, int quantums) {
    pcb->pid = pid;
    pcb->sp = 0; // Initialize stack pointer (to be set when process is created)
    pcb->pc = 0; // Initialize program counter (to be set when process is created)
    pcb->lr = 0; // Initialize link register
    pcb->spsr = 0; // Initialize saved program status register
    for (int i = 0; i < 13; i++) {
        pcb->registers[i] = 0; // Initialize general-purpose registers
    }
    pcb->name = 0;
    pcb->entry = 0;
    pcb->state = READY; // Initialize process state (READY)
    pcb->max_quantums = quantums;
    pcb->curr_quantums = 0;
}

void configure_process(PCB *pcb, const char *name, process_entry_t entry) {
    pcb->name = name;
    pcb->entry = entry;
}

void setup_initial_process_stack(PCB *pcb, unsigned int stack_top) {
    volatile unsigned int *frame = (volatile unsigned int *)(stack_top - (INITIAL_CONTEXT_WORDS * sizeof(unsigned int)));

    for (unsigned int i = 0; i < 13u; i++) {
        frame[i] = 0u;
        pcb->registers[i] = 0;
    }

    frame[13] = (unsigned int)pcb->entry;

    pcb->sp = (int)frame;
    pcb->pc = (int)pcb->entry;
    pcb->lr = (int)pcb->entry;
    pcb->spsr = 0;
}

// Save from IRQ frame into PCB
void save_process_state(PCB *pcb, StackFrame *frame, int is_irq, int original_sp) {
    for (int i = 0; i < 13; i++)
        pcb->registers[i] = frame->r[i];
    pcb->pc = frame->lr;
    pcb->lr = frame->lr;
    pcb->sp = is_irq? read_svc_sp() : original_sp;
}

// Restore from PCB into IRQ frame so ldmfd picks it up
void restore_process_state(PCB *pcb, StackFrame *frame) {
    for (int i = 0; i < 13; i++)
        frame->r[i] = pcb->registers[i];
    frame->lr = pcb->pc;   // subs pc, lr, #0 will jump here on return
    write_svc_sp(pcb->sp);
}

void set_process_state(PCB *pcb, int state) {
    pcb->state = state;
}