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
    @ 2. Switch to SVC mode and set up the main stack.
    @    0xD3 = 0b11010011: SVC mode (0x13) | FIQ disabled | IRQ disabled
    @    IRQs are still disabled here — enable_irq() is called from C after
    @    all peripherals are fully initialized.
    @ -----------------------------------------------------------------------
    msr cpsr_c, #0xD3
    ldr sp, =_stack_top

    @ -----------------------------------------------------------------------
    @ 3. Set the Vector Base Address Register (VBAR) to point at our table.
    @    cp15 c12 c0 0 is the VBAR on Cortex-A8 (ARMv7-A).
    @    This is what allows the vector table to live at an arbitrary address
    @    instead of being fixed at 0x00000000 as on older ARM cores.
    @ -----------------------------------------------------------------------
    ldr r0, =vector_table
    mcr p15, 0, r0, c12, c0, 0

    @ -----------------------------------------------------------------------
    @ 4. Zero the .bss section.
    @    The C standard requires uninitialized globals to be zero. The
    @    bootloader on BeagleBone may not do this, so we do it ourselves.
    @    _bss_start and _bss_end are symbols defined in the linker script.
    @ -----------------------------------------------------------------------
    ldr r0, =_bss_start
    ldr r1, =_bss_end
    mov r2, #0
bss_zero_loop:
    cmp  r0, r1
    strlt r2, [r0], #4
    blt  bss_zero_loop

    @ -----------------------------------------------------------------------
    @ 5. Jump to C entry point.
    @ -----------------------------------------------------------------------
    bl main

    @ -----------------------------------------------------------------------
    @ If main() ever returns (it shouldn't), halt the CPU here.
    @ -----------------------------------------------------------------------
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

swi_handler:
    b hang

prefetch_handler:
    b hang

data_handler:
    b hang

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

@ Symbols used by reset_handler to zero .bss
.globl _bss_start
_bss_start:

@ IRQ mode stack — used exclusively during IRQ exception handling
_irq_stack_bottom:
    .skip 0x1000                @ 4KB — sufficient for nested C handler calls
_irq_stack_top:

@ SVC mode stack — used by main() and all normal C code
_stack_bottom:
    .skip 0x2000                @ 8KB main stack
_stack_top:

.globl _bss_end
_bss_end: