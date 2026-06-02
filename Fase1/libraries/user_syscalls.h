#ifndef USER_SYSCALLS_H
#define USER_SYSCALLS_H

enum SyscallID {
    SYS_YIELD = 0,
    SYS_EXIT = 1,
    SYS_WRITE = 2
};

static inline int syscall(int id, int arg1, int arg2, int arg3) {
    register int r0 asm("r0") = id;
    register int r1 asm("r1") = arg1;
    register int r2 asm("r2") = arg2;
    register int r3 asm("r3") = arg3;
    asm volatile (
        "svc #0\n" // Trigger the system call interrupt
        : "+r" (r0) // Output: store return value in r0
        : "r" (r1), "r" (r2), "r" (r3) // Input: syscall number and arguments
        : "memory" // Clobber memory to prevent compiler optimizations
    );
    return r0; // Return the result of the system call
}

static inline int yield() {
    return syscall(SYS_YIELD, 0, 0, 0); // Syscall ID for yield
}

static inline int exit(int status) {
    return syscall(SYS_EXIT, status, 0, 0); // Syscall ID for exit
}

static inline int write(int fd, const void* buf, unsigned int size) {
    return syscall(SYS_WRITE, fd, (int)buf, size); // Syscall ID for write
}

#endif // USER_SYSCALLS_H
