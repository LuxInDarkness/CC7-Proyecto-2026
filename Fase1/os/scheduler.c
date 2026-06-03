#include "scheduler.h"
#include "os_io.h"

static ProcessQueue process_queue;
static PCB *active_process = 0;

ProcessQueue * get_process_queue(void) {
    return &process_queue;
}

PCB ** get_active_process(void) {
    return &active_process;
}

// Initializes the empty queue
void init_queue (ProcessQueue * queue) {
    queue->running_index = 0;
    queue->ready_index = 0;
    queue->blocked_index = 0;
    queue->terminated_index = 0;
}

// Add process to queue based on its current state
void enqueue_process(ProcessQueue * queue, PCB * process) {
    if (process->state == RUNNING) {
        add_to_queue(queue->running_pool, &queue->running_index, process);
    } else if (process->state == READY) {
        add_to_queue(queue->ready_pool, &queue->ready_index, process);
    } else if (process->state == BLOCKED) {
        add_to_queue(queue->blocked_pool, &queue->blocked_index, process);
    } else if (process->state == TERMINATED) {
        add_to_queue(queue->terminated_pool, &queue->terminated_index, process);
    } else {
        // Solo activar de ser necesario
        // print("Al agregar, estado de proceso invalido: %d\n", process->state);
    }
}

// Remove process from its respective queue, must be done before updating its state
PCB remove_process(ProcessQueue * queue, PCB * process) {
    if (process->state == RUNNING) {
        return remove_from_queue(queue->running_pool, &queue->running_index, process);
    } else if (process->state == READY) {
        return remove_from_queue(queue->ready_pool, &queue->ready_index, process);
    } else if (process->state == BLOCKED) {
        return remove_from_queue(queue->blocked_pool, &queue->blocked_index, process);
    } else if (process->state == TERMINATED) {
        return remove_from_queue(queue->terminated_pool, &queue->terminated_index, process);
    } else {
        // Solo activar de ser necesario
        // print("Al remover, estado de proceso invalido: %d\n", process->state);
        PCB empty;
        empty.pid = -1;
        return empty;
    }
}

// Adds the new process to the given queue
void add_to_queue(PCB * pool, int * index, PCB * process) {
    if (*index >= MAX_PROCESSES) {
        // Solo activar de ser necesario
        // print("Error: pool lleno, no se puede agregar proceso %d\n", process->pid);
        return;
    }

    pool[*index] = *process;   // copy PCB into the next free slot
    (*index)++;
}

// Removes the given process from the given queue based on the PID
PCB remove_from_queue(PCB * pool, int * index, PCB * process) {
    int i;
    int found = -1;
    PCB removed;
    removed.pid = -1;

    // Find the process matching this PID
    for (i = 0; i < *index; i++) {
        if (pool[i].pid == process->pid) {
            found = i;
            break;
        }
    }

    if (found == -1) {
        // Solo activar si debuggear es necesario
        // print("Error: proceso %d con estado %d no encontrado en la cola\n", process->pid, process->state);
        return removed;
    }

    removed = pool[found];

    // Shift everything after the removed process one position left
    for (i = found; i < *index - 1; i++) {
        pool[i] = pool[i + 1];
    }

    (*index)--;
    return removed;
}

// Returns the next process in the pool and removes it from the pool, returns pid -1 if pool is empty
PCB get_next_process(PCB * pool, int * index) {
    if (*index == 0) {
        // Solo activar de ser necesario
        // print("Error: pool vacio\n");
        PCB empty;
        empty.pid = -1;
        return empty;
    }

    PCB process = pool[0];
    remove_from_queue(pool, index, &process);
    return process;
}

// Find a PCB by PID in a pool
PCB * find_in_pool(PCB * pool, int index, int pid) {
    int i;
    for (i = 0; i < index; i++) {
        if (pool[i].pid == pid) return &pool[i];
    }
    return 0;
}

void move_process(PCB * pcb, int new_state) {
    PCB removed = remove_process(&process_queue, pcb);
    if (removed.pid == -1) return;
    set_process_state(&removed, new_state);
    enqueue_process(&process_queue, &removed);
}

PCB * create_process(int pid, const char *name, process_entry_t entry, unsigned int stack_top, int quantums) {
    if (process_queue.ready_index >= MAX_PROCESSES) {
        print("Error: no hay espacio en ready pool\n");
        return 0;
    }

    PCB *pcb = &process_queue.ready_pool[process_queue.ready_index];
    initialize_pcb(pcb, pid, quantums);
    configure_process(pcb, name, entry);
    setup_initial_process_stack(pcb, stack_top);
    process_queue.ready_index++;

    return pcb;
}