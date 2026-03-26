#ifndef PCB_H
#define PCB_H

struct PCB;
typedef void (*process_entry_t)(struct PCB *pcb);

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
    const char *name; // Human-readable process name
    int max_quantums; // Number of timer operations that the process will be allowed to run
    int curr_quantums;
    process_entry_t entry; // Kernel-managed process entry point
} PCB;

typedef struct IRQFrame {
    int r[13];    // r0-r12
    int lr;       // adjusted return address
} IRQFrame;

extern int read_svc_sp(void);
extern void write_svc_sp(int sp);
void initialize_pcb(PCB *pcb, int pid, int quantums);
void configure_process(PCB *pcb, const char *name, process_entry_t entry);
void setup_initial_process_stack(PCB *pcb, unsigned int stack_top);
void save_process_state(PCB *pcb, IRQFrame *frame);
void restore_process_state(PCB *pcb, IRQFrame *frame);
void set_process_state(PCB *pcb, int state);

#endif // PCB_H