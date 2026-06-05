#include "svc.h"

// Safer version for ARMv7-A
int read_svc_sp(void) {
    int sp;
    __asm__ volatile (
        "dsb\n"             // Data Syncronization Barrier to avoid cache
        "cps #0x13\n"       // switch to SVC mode (0x13)
        "mov %0, sp\n"      // read SP_svc
        "cps #0x12\n"       // switch back to IRQ mode (0x12)
        : "=r"(sp) : : "memory"
    );
    return sp;
}

void write_svc_sp(int sp) {
    __asm__ volatile (
        "cps #0x13\n"       // switch to SVC mode
        "mov sp, %0\n"      // write SP_svc
        "cps #0x12\n"       // switch back to IRQ mode
        : : "r"(sp) : "memory"
    );
}

void write_svc_sp_from_svc(int sp) {
    __asm__ volatile (
        "mov sp, %0\n"
        : : "r"(sp) : "memory"
    );
}

int read_spsr_svc(void) {
    int spsr;
    __asm__ volatile("mrs %0, spsr" : "=r"(spsr));
    return spsr;
}

int read_usr_sp(void) {
    int sp;
    __asm__ volatile (
        "mrs r1, cpsr\n"          // save current CPSR
        "cpsid i, #0x10\n"        // disable IRQ, switch to USR mode
        "mov %0, sp\n"            // read SP_usr
        "msr cpsr_c, r1\n"        // restore original CPSR (mode + I/F flags)
        : "=r"(sp) : : "r1", "memory"
    );
    return sp;
}

void write_usr_sp(int sp) {
    __asm__ volatile (
        "mrs r1, cpsr\n"
        "cpsid i, #0x10\n"
        "mov sp, %0\n"
        "msr cpsr_c, r1\n"
        : : "r"(sp) : "r1", "memory"
    );
}
