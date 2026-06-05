#include "pcb.h"

int next_spsr = 0;

#define INITIAL_CONTEXT_WORDS 14u

void initialize_pcb(PCB *pcb, int pid, int quantums) {
    pcb->pid = pid;
    pcb->sp = 0; // Initialize stack pointer (to be set when process is created)
    pcb->pc = 0; // Initialize program counter (to be set when process is created)
    pcb->lr = 0; // Initialize link register
    pcb->spsr = 0; // Initialize saved program status register
    pcb->cpsr = 0; // Initialize current program status register
    for (int i = 0; i < 13; i++) {
        pcb->registers[i] = 0; // Initialize general-purpose registers
    }
    pcb->name = 0;
    pcb->entry = 0;
    pcb->state = READY; // Initialize process state (READY)
    pcb->max_quantums = quantums;
    pcb->curr_quantums = 0;
    pcb->syscall_id = -1;
    pcb->termination_status = -1;
    pcb->fault_type = FAULT_NONE;
    pcb->fault_address = 0;
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
    
    pcb->spsr = 0x10;   // USR mode
    pcb->cpsr = 0;
}

// Save interrupted user context from exception frame into PCB.
// Called from IRQ, SVC, or ABT handlers while still in privileged mode.
// Reads actual SP_usr and the exception-mode SPSR (which holds the user CPSR).
void save_process_state(PCB *pcb, StackFrame *frame) {
    for (int i = 0; i < 13; i++)
        pcb->registers[i] = frame->r[i];
    pcb->pc = frame->lr;
    pcb->lr = frame->lr;
    pcb->sp   = read_usr_sp();      // actual USR stack pointer
    pcb->spsr = read_spsr_svc();    // user CPSR at exception entry
}

// Restore PCB context into the exception frame and prepare for return to USR.
// Writes SP_usr directly (not through a proxy) and sets next_spsr for the
// assembly exception-return sequence.
void restore_process_state(PCB *pcb, StackFrame *frame) {
    for (int i = 0; i < 13; i++)
        frame->r[i] = pcb->registers[i];
    frame->lr = pcb->pc;            // exception return will jump here
    write_usr_sp(pcb->sp);          // set actual SP_usr for the process
    next_spsr = pcb->spsr;          // used by asm to override SPSR before return
}

void set_process_state(PCB *pcb, int state) {
    pcb->state = state;
}