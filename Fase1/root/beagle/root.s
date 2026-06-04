.section .text
.syntax unified
.code 32
.globl _start
_start:

@ ===========================================================================
@ Exception Vector Table
@ On Cortex-A8 (ARMv7-A) the vector table can live anywhere in memory.
@ Its base address is written to the VBAR register in reset_handler.
@ Each entry uses a simple branch — valid since all handlers are in the
@ same section and within the ±32MB branch range.
@ Must be 32-byte aligned (.align 5).
@ ===========================================================================
.align 5
vector_table:
    b reset_handler      @ 0x00: Reset
    b undefined_handler  @ 0x04: Undefined Instruction
    b swi_handler        @ 0x08: Software Interrupt (SWI)
    b prefetch_handler   @ 0x0C: Prefetch Abort
    b data_handler       @ 0x10: Data Abort
    b .                  @ 0x14: Reserved — loop in place
    b irq_handler        @ 0x18: IRQ (Interrupt Request)
    b fiq_handler        @ 0x1C: FIQ (Fast Interrupt Request)

@ ===========================================================================
@ Reset Handler — entry point after power-on or hardware reset
@ ===========================================================================
reset_handler:
    @ -----------------------------------------------------------------------
    @ 1. Set up IRQ mode stack.
    @    The CPU has banked registers per mode — SP_irq is a separate register
    @    from SP_svc. We must explicitly switch to IRQ mode and initialize it,
    @    otherwise the first IRQ will use an uninitialized stack pointer and
    @    corrupt memory.
    @    0xD2 = 0b11010010: IRQ mode (0x12) | FIQ disabled | IRQ disabled
    @ -----------------------------------------------------------------------
    msr cpsr_c, #0xD2
    ldr sp, =_irq_stack_top

    @ -----------------------------------------------------------------------
    @ 2. Set up ABT (Abort) mode stack.
    @    0xD7 = 0b11010111: ABT mode (0x17) | FIQ disabled | IRQ disabled
    @    The abort handlers use this stack to save user context before
    @    calling the C fault handler. Each abort pushes 14 registers (56
    @    bytes), so 1KB is more than enough.
    @ -----------------------------------------------------------------------
    msr cpsr_c, #0xD7
    ldr sp, =_abt_stack_top

    @ -----------------------------------------------------------------------
    @ 3. Switch to SVC mode and set up the main stack.
    @    0xD3 = 0b11010011: SVC mode (0x13) | FIQ disabled | IRQ disabled
    @    IRQs are still disabled here — enable_irq() is called from C after
    @    all peripherals are fully initialized.
    @ -----------------------------------------------------------------------
    msr cpsr_c, #0xD3
    ldr sp, =_stack_top

    @ -----------------------------------------------------------------------
    @ 4. Set the Vector Base Address Register (VBAR) to point at our table.
    @    cp15 c12 c0 0 is the VBAR on Cortex-A8 (ARMv7-A).
    @    This is what allows the vector table to live at an arbitrary address
    @    instead of being fixed at 0x00000000 as on older ARM cores.
    @ -----------------------------------------------------------------------
    ldr r0, =vector_table
    mcr p15, 0, r0, c12, c0, 0

    @ -----------------------------------------------------------------------
    @ 5. Zero the .bss section.
    @    The C standard requires uninitialized globals to be zero. The
    @    bootloader on BeagleBone may not do this, so we do it ourselves.
    @    __bss_start__ and __bss_end__ are symbols defined in the linker script.
    @ -----------------------------------------------------------------------
    ldr r0, =__bss_start__
    ldr r1, =__bss_end__
    mov r2, #0
bss_zero_loop:
    cmp  r0, r1
    strlt r2, [r0], #4
    blt  bss_zero_loop

    @ -----------------------------------------------------------------------
    @ 6. Jump to C entry point.
    @ -----------------------------------------------------------------------
    bl main

    @ -----------------------------------------------------------------------
    @ If main() ever returns (it shouldn't), halt the CPU here.
    @ -----------------------------------------------------------------------
.globl hang
hang:
    b hang

@ ===========================================================================
@ Fault Handlers
@ These halt execution. In a production system each would save CPU state
@ and report the fault, but for bare metal development looping is sufficient
@ to make faults visible in a debugger.
@ ===========================================================================
undefined_handler:
    b hang

@ ===========================================================================
@ SWI Handler
@
@ ARM SWI entry conditions (ARMv7-A):
@ LR_svc = address of instruction after the SWI (correct return address)
@ No -4 adjustment needed unlike IRQ
@ ===========================================================================
swi_handler:
    stmfd sp!, {r0-r12, lr}     @ push frame (56 bytes), sp = frame pointer
    mov   r0, sp                @ arg1: frame*
    add   r1, sp, #56           @ arg2: original_sp
    bl    swi_c_handler         @ r0 = next process SP on return

    @ Store next-SP at original_sp (one word above frame top).
    @ After ldmfd restores sp to original_sp, we load it back.
    str   r0, [sp, #56]         @ *original_sp = next process SP
    ldmfd sp!, {r0-r12, lr}     @ restore frame, sp is now original_sp
                                @ lr = next process PC (written by swi_c_handler)
    ldr   sp, [sp]              @ sp = *original_sp = next process SP
    movs  pc, lr                @ exception return, jump to next process

@ ===========================================================================
@ Prefetch Abort Handler
@
@ ARM abort entry conditions:
@   - CPU switches to ABT mode automatically
@   - LR_abt = fault address + 4 (prefetch) or + 8 (data abort)
@   - CPSR saved to SPSR_abt, IRQs masked
@
@ The handler saves the full user context (r0-r12, adjusted LR) on the
@ ABT stack, reads the CP15 fault registers (IFSR/IFAR for prefetch,
@ DFSR/DFAR for data), and calls fault_c_handler() in C. The C handler
@ classifies the fault, terminates the active process, and switches to
@ the next ready process. If no ready process exists, the CPU idles
@ using os_idle_sp.
@
@ Return path: same as SWI handler — the C handler writes the next
@ process's context into the stack frame and returns its SP. We store
@ the returned SP at original_sp, restore the frame via ldmfd, load
@ the new SP, and do a standard exception return with subs pc, lr, #0.
@ ===========================================================================
prefetch_handler:
    sub   lr, lr, #4            @ LR_abt = fault address + 4, subtract 4
    stmfd sp!, {r0-r12, lr}     @ Save user context on abort stack (56 bytes)
    mrc   p15, 0, r2, c5, c0, 1 @ Read IFSR (Instruction Fault Status Register)
    mrc   p15, 0, r3, c6, c0, 2 @ Read IFAR (Instruction Fault Address Register)
    mov   r0, sp                @ arg0: StackFrame*
    mov   r1, r2                @ arg1: FSR
    mov   r2, r3                @ arg2: FAR
    mov   r3, #1                @ arg3: is_prefetch = 1
    bl    fault_c_handler       @ returns next process SP in r0
    str   r0, [sp, #56]         @ Store next SP at original_sp position
    ldmfd sp!, {r0-r12, lr}     @ Restore frame (now has next process context)
    ldr   sp, [sp]              @ SP = next process stack pointer
    subs  pc, lr, #0            @ Exception return — CPSR = SPSR_abt, PC = LR

@ ===========================================================================
@ Data Abort Handler
@
@ Same logic as prefetch_handler but reads DFSR/DFAR and adjusts LR_abt by
@ -8 (data aborts leave LR = fault address + 8).
@ ===========================================================================
data_handler:
    sub   lr, lr, #8            @ LR_abt = fault address + 8, subtract 8
    stmfd sp!, {r0-r12, lr}     @ Save user context on abort stack
    mrc   p15, 0, r2, c5, c0, 0 @ Read DFSR (Data Fault Status Register)
    mrc   p15, 0, r3, c6, c0, 0 @ Read DFAR (Data Fault Address Register)
    mov   r0, sp                @ arg0: StackFrame*
    mov   r1, r2                @ arg1: FSR
    mov   r2, r3                @ arg2: FAR
    mov   r3, #0                @ arg3: is_prefetch = 0
    bl    fault_c_handler       @ returns next process SP in r0
    str   r0, [sp, #56]         @ Store next SP at original_sp position
    ldmfd sp!, {r0-r12, lr}     @ Restore frame
    ldr   sp, [sp]              @ SP = next process stack pointer
    subs  pc, lr, #0            @ Exception return — CPSR = SPSR_abt, PC = LR

fiq_handler:
    b hang

@ ===========================================================================
@ IRQ Handler
@
@ ARM IRQ entry conditions (ARMv7-A):
@   - CPU automatically switches to IRQ mode
@   - LR_irq = PC + 4 of the interrupted instruction (one instruction ahead)
@   - CPSR is saved to SPSR_irq, IRQs are masked
@
@ Return: "subs pc, lr, #0" with destination=pc and S flag set in a
@ privileged mode causes the CPU to atomically restore CPSR from SPSR_irq
@ and branch to LR — this is the ARMv7-A exception return mechanism.
@ ===========================================================================
irq_handler:
    sub   lr, lr, #4            @ Correct LR: undo the +4 offset added by hardware
    stmfd sp!, {r0-r12, lr}     @ Save full context (r0-r12 + corrected LR) onto IRQ stack

    mov   r0, sp                @ pass IRQ stack pointer as argument to C
    bl timer_irq_handler        @ Call C handler — must clear the timer interrupt flag
                                @ and acknowledge the interrupt at the INTCPS controller

    ldmfd sp!, {r0-r12, lr}     @ Restore full context
    subs  pc, lr, #0            @ Exception return: restore CPSR from SPSR_irq, branch to LR

@ ===========================================================================
@ Low-level Memory Access
@
@ These are called from C to perform 32-bit register reads and writes.
@ Using explicit functions ensures the compiler does not optimize away
@ or reorder accesses to memory-mapped peripheral registers.
@
@ PUT32(unsigned int addr, unsigned int value) — write value to addr
@ GET32(unsigned int addr) → unsigned int     — read from addr
@ ===========================================================================
.globl PUT32
PUT32:
    str r1, [r0]                @ *r0 = r1
    bx  lr

.globl GET32
GET32:
    ldr r0, [r0]                @ r0 = *r0
    bx  lr

@ ===========================================================================
@ Enable IRQs
@
@ Clears the I bit (bit 7) in the CPSR, allowing IRQ exceptions to fire.
@ Should only be called after all peripheral initialization is complete
@ (GIC/INTCPS unmasked, timer configured) to avoid spurious interrupts.
@ ===========================================================================
.globl enable_irq
enable_irq:
    mrs r0, cpsr
    bic r0, r0, #0x80           @ Clear bit 7 (I bit): 0=IRQs enabled, 1=disabled
    msr cpsr_c, r0              @ Write back control byte only (_c field)
    bx  lr

@ ===========================================================================
@ Stack Allocation (.bss — zeroed at startup by reset_handler)
@
@ Two separate stacks are required because IRQ mode uses a banked SP_irq
@ register that is entirely independent from the SVC mode SP_svc.
@ ===========================================================================
.section .bss
.align 4

@ IRQ mode stack — used exclusively during IRQ exception handling
_irq_stack_bottom:
    .skip 0x1000                @ 4KB — sufficient for nested C handler calls
_irq_stack_top:

@ ABT mode stack — used by prefetch and data abort handlers
.align 4
_abt_stack_bottom:
    .skip 0x400                 @ 1KB abort stack (14 regs × 4 = 56 bytes per entry)
_abt_stack_top:

@ SVC mode stack — used by main() and all normal C code
_stack_bottom:
    .skip 0x2000                @ 8KB main stack
_stack_top:
