#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "pcb.h"

#define MAX_PROCESSES 20

typedef struct ProcessQueue {
    PCB ready_pool[MAX_PROCESSES];
    int ready_index;
    PCB running_pool[MAX_PROCESSES];
    int running_index;
    PCB blocked_pool[MAX_PROCESSES];
    int blocked_index;
    PCB terminated_pool[MAX_PROCESSES];
    int terminated_index;
} ProcessQueue;

void init_queue(ProcessQueue * queue);
void enqueue_process(ProcessQueue * queue, PCB * process);
PCB remove_process(ProcessQueue * queue, PCB * process);
void add_to_queue(PCB * pool, int * index, PCB * process);
PCB remove_from_queue(PCB * pool, int * index, PCB * process);
PCB get_next_process(PCB * pool, int * index);
PCB * find_in_pool(PCB * pool, int index, int pid);

#endif // SCHEDULER_H