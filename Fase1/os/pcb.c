#include "pcb.h"

void initialize_pcb(PCB *pcb, int pid) {
    pcb->pid = pid;
    pcb->sp = 0; // Initialize stack pointer (to be set when process is created)
    pcb->pc = 0; // Initialize program counter (to be set when process is created)
    pcb->lr = 0; // Initialize link register
    pcb->spsr = 0; // Initialize saved program status register
    for (int i = 0; i < 13; i++) {
        pcb->registers[i] = 0; // Initialize general-purpose registers
    }
    pcb->state = READY; // Initialize process state (e.g., READY)
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