#include "svc.h"

// Safer version for ARMv7-A
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

void write_svc_sp(int sp) {
    __asm__ volatile (
        "mrs r1, cpsr\n"
        "orr r2, r1, #0xC0\n"
        "bic r2, r2, #0x1F\n"
        "orr r2, r2, #0x13\n"
        "msr cpsr_c, r2\n"
        "mov sp, %0\n"
        "msr cpsr_c, r1\n"
        : : "r"(sp) : "r1", "r2", "memory"
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
    __asm__ volatile (
        "mrs %0, spsr\n"    // reads SPSR of current mode (SPSR_svc or SPSR_irq)
        : "=r"(spsr) : : "memory"
    );
    return spsr;
}

// In svc.c — write SP_usr (for processes running in USR mode)
void write_usr_sp(int sp) {
    __asm__ volatile (
        "mrs r1, cpsr\n"
        "orr r2, r1, #0xC0\n"
        "bic r2, r2, #0x1F\n"
        "orr r2, r2, #0x1F\n"    // SYS mode (0x1F) — shares registers with USR
        "msr cpsr_c, r2\n"
        "mov sp, %0\n"
        "msr cpsr_c, r1\n"
        : : "r"(sp) : "r1", "r2", "memory"
    );
}

int read_usr_sp(void) {
    int sp;
    __asm__ volatile (
        "mrs r1, cpsr\n"
        "orr r2, r1, #0xC0\n"
        "bic r2, r2, #0x1F\n"
        "orr r2, r2, #0x1F\n"    // SYS mode
        "msr cpsr_c, r2\n"
        "mov %0, sp\n"
        "msr cpsr_c, r1\n"
        : "=r"(sp) : : "r1", "r2", "memory"
    );
    return sp;
}
