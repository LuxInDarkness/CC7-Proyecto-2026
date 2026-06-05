#include "fault.h"
#include "scheduler.h"
#include "os_io.h"

extern int os_idle_sp;

// classify_fault — decode ARM FSR into a FaultType
FaultType classify_fault(unsigned int fsr) {
    unsigned int status = fsr & 0xF; // FSR[3:0]

    switch (status) {
        case 0x1:                        // 0b0001
            return FAULT_ALIGNMENT;
        case 0x5:                        // 0b0101
        case 0x7:                        // 0b0111
            return FAULT_TRANSLATION;
        case 0x9:                        // 0b1001
        case 0xB:                        // 0b1011
            return FAULT_DOMAIN;
        case 0xD:                        // 0b1101
        case 0xF:                        // 0b1111
            return FAULT_PERMISSION;
        case 0x8:                        // 0b1000
        case 0xC:                        // 0b1100
        case 0xE:                        // 0b1110
            return FAULT_EXTERNAL;
        default:
            return FAULT_UNKNOWN;
    }
}

static const char *fault_type_name(FaultType type) {
    switch (type) {
        case FAULT_NONE:        return "NONE";
        case FAULT_ALIGNMENT:   return "ALIGNMENT";
        case FAULT_TRANSLATION: return "TRANSLATION";
        case FAULT_DOMAIN:      return "DOMAIN";
        case FAULT_PERMISSION:  return "PERMISSION";
        case FAULT_EXTERNAL:    return "EXTERNAL";
        case FAULT_UNKNOWN:
        default:                return "UNKNOWN";
    }
}

// fault_c_handler — C-level abort handler
//
// Called from the assembly prefetch and data abort handlers after they have
// saved the user context (r0-r12, lr) on the abort stack and read the CP15
// fault status and address registers.
int fault_c_handler(void *frame, unsigned int fsr, unsigned int far, int is_prefetch) {
    StackFrame *f = (StackFrame *)frame;
    FaultType type = classify_fault(fsr);

    // === TRACE: USER_TO_KERNEL fault ===
    if (ACTIVE_PROCESS != 0) {
        print("MODE_SWITCH USER_TO_KERNEL pid=%d reason=fault type=%s\n",
              ACTIVE_PROCESS->pid, fault_type_name(type));
    }

    // Diagnostic message
    if (ACTIVE_PROCESS != 0) {
        print("FAULT: pid=%d type=%s addr=0x%X %s\n",
              ACTIVE_PROCESS->pid,
              fault_type_name(type),
              far,
              is_prefetch ? "(prefetch)" : "(data)");
    } else {
        print("FAULT: type=%s addr=0x%X %s (no active process)\n",
              fault_type_name(type),
              far,
              is_prefetch ? "(prefetch)" : "(data)");
    }

    // Save process state, record fault info, and terminate
    if (ACTIVE_PROCESS != 0) {
        save_process_state(ACTIVE_PROCESS, f);
        ACTIVE_PROCESS->fault_type = type;
        ACTIVE_PROCESS->fault_address = far;
        ACTIVE_PROCESS->termination_status = -1; // terminated by fault
        move_process(ACTIVE_PROCESS, TERMINATED);
        ACTIVE_PROCESS = 0;
    }

    // Switch to next ready process if one exists
    if (QUEUE->ready_index > 0) {
        PCB *next_ready = &QUEUE->ready_pool[0];
        move_process(next_ready, RUNNING);
        next_ready = 0;

        ACTIVE_PROCESS = &QUEUE->running_pool[QUEUE->running_index - 1];

        // Restore next process's context into the abort stack frame
        restore_process_state(ACTIVE_PROCESS, f);

        // === TRACE: KERNEL_TO_USER fault_recovery ===
        if (ACTIVE_PROCESS != 0) {
            print("MODE_SWITCH KERNEL_TO_USER pid=%d reason=fault_recovery\n",
                  ACTIVE_PROCESS->pid);
        }

        return ACTIVE_PROCESS->sp;
    }

    // No ready processes — set frame->lr to a safe halt point and return os_idle_sp
    extern void hang(void);
    f->lr = (int)hang;
    return os_idle_sp;
}
