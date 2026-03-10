#ifndef PCB_H
#define PCB_H

#define RUNNING 0
#define READY 1
#define BLOCKED 2
#define TERMINATED 3

typedef struct PCB {
    // Process Control Block structure
    int pid; // Process ID
    int sp;  // Stack pointer
    int pc;  // Program counter
    int lr;  // Link register
    int spsr; // Saved Program Status Register
    int registers[13]; // General-purpose registers (r0-r12)
    int state; // Process state (e.g., READY, RUNNING, BLOCKED)
    // Add more fields as necessary (e.g., priority, scheduling info, etc.)
} PCB;

void initialize_pcb(PCB *pcb, int pid);
void save_process_state(PCB *pcb, int sp, int pc, int lr, int spsr, int *registers);
void restore_process_state(PCB *pcb, int *sp, int *pc, int *lr, int *spsr, int *registers);
void set_process_state(PCB *pcb, int state);

#endif // PCB_H