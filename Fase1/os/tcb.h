#ifndef TCB_H
#define TCB_H

typedef struct TCB {
    // Thread Control Block structure (can be expanded as needed)
    int tid; // Thread ID
    // Add more fields as necessary (e.g., state, registers, etc.)
} TCB;

void initialize_tcb(TCB *tcb, int tid);

#endif // TCB_H