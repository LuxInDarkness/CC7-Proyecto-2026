#ifndef FAULT_H
#define FAULT_H

#include "pcb.h"

FaultType classify_fault(unsigned int fsr);

int fault_c_handler(void *frame, unsigned int fsr, unsigned int far, int is_prefetch);

#endif // FAULT_H
