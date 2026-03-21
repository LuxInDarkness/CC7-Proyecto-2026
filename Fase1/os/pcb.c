#include "pcb.h"

#define INITIAL_CONTEXT_WORDS 14u

void initialize_pcb(PCB *pcb, int pid) {
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

void save_process_state(PCB *pcb, int sp, int pc, int lr, int spsr, int *registers) {
    pcb->sp = sp;
    pcb->pc = pc;
    pcb->lr = lr;
    pcb->spsr = spsr;
    for (int i = 0; i < 13; i++) {
        pcb->registers[i] = registers[i]; // Save general-purpose registers
    }
}

void restore_process_state(PCB *pcb, int *sp, int *pc, int *lr, int *spsr, int *registers) {
    *sp = pcb->sp;
    *pc = pcb->pc;
    *lr = pcb->lr;
    *spsr = pcb->spsr;
    for (int i = 0; i < 13; i++) {
        registers[i] = pcb->registers[i]; // Restore general-purpose registers
    }
}

void set_process_state(PCB *pcb, int state) {
    pcb->state = state;
}