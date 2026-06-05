#include "svc.h"

void write_svc_sp(int sp) {
    __asm__ volatile (
        "mrs r1, cpsr\n"          // save current IRQ mode CPSR
        "orr r2, r1, #0xC0\n"    // set I and F bits (disable IRQ+FIQ)
        "bic r2, r2, #0x1F\n"    // clear mode bits
        "orr r2, r2, #0x13\n"    // set SVC mode
        "msr cpsr_c, r2\n"        // switch to SVC with IRQs disabled
        "mov sp, %0\n"            // write SP_svc
        "msr cpsr_c, r1\n"        // restore original CPSR (back to IRQ mode)
        : : "r"(sp) : "r1", "r2", "memory"
    );
}

int read_svc_sp(void) {
    int sp;
    __asm__ volatile (
        "mrs r1, cpsr\n"
        "orr r2, r1, #0xC0\n"
        "bic r2, r2, #0x1F\n"
        "orr r2, r2, #0x13\n"
        "msr cpsr_c, r2\n"
        "mov %0, sp\n"
        "msr cpsr_c, r1\n"
        : "=r"(sp) : : "r1", "r2", "memory"
    );
    return sp;
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
        "orr r2, r1, #0xC0\n"     // disable IRQ+FIQ
        "bic r2, r2, #0x1F\n"     // clear mode bits
        "orr r2, r2, #0x10\n"     // USR mode (0x10)
        "msr cpsr_c, r2\n"        // switch to USR with IRQs disabled
        "mov %0, sp\n"            // read SP_usr
        "msr cpsr_c, r1\n"        // restore original CPSR
        : "=r"(sp) : : "r1", "r2", "memory"
    );
    return sp;
}

void write_usr_sp(int sp) {
    __asm__ volatile (
        "mrs r1, cpsr\n"
        "orr r2, r1, #0xC0\n"
        "bic r2, r2, #0x1F\n"
        "orr r2, r2, #0x10\n"
        "msr cpsr_c, r2\n"
        "mov sp, %0\n"
        "msr cpsr_c, r1\n"
        : : "r"(sp) : "r1", "r2", "memory"
    );
}
