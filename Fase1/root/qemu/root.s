.section .vectors, "ax"
.syntax unified
.code 32
.globl _start
_start:

@ ===========================================================================
@ Exception Vector Table
@ MUST be at 0x00000000 on VersatilePB (ARM926EJ-S has no VBAR register).
@ The linker script must place .vectors section at 0x00000000.
@ ===========================================================================
vector_table:
    ldr pc, =reset_handler      @ 0x00: Reset
    ldr pc, =undefined_handler  @ 0x04: Undefined Instruction
    ldr pc, =swi_handler        @ 0x08: Software Interrupt (SWI)
    ldr pc, =prefetch_handler   @ 0x0C: Prefetch Abort
    ldr pc, =data_handler       @ 0x10: Data Abort
    ldr pc, =.                  @ 0x14: Reserved (loop)
    ldr pc, =irq_handler        @ 0x18: IRQ
    ldr pc, =fiq_handler        @ 0x1C: FIQ

@ ---------------------------------------------------------------------------
@ Jump table — ldr pc, =label loads from here, giving full 32-bit range.
@ Placed immediately after vectors so they are always reachable.
@ ---------------------------------------------------------------------------
_vectors_jump_table:
    .word reset_handler
    .word undefined_handler
    .word swi_handler
    .word prefetch_handler
    .word data_handler
    .word 0
    .word irq_handler
    .word fiq_handler

@ ===========================================================================
@ Code section
@ ===========================================================================
.section .text
.code 32

reset_handler:
    @ -----------------------------------------------------------------------
    @ 1. Set up IRQ mode stack
    @    Switch to IRQ mode (0x12), disable IRQ+FIQ, set IRQ stack pointer
    @ -----------------------------------------------------------------------
    msr cpsr_c, #0xD2           @ IRQ mode | FIQ disabled | IRQ disabled
    ldr sp, =_irq_stack_top

    @ -----------------------------------------------------------------------
    @ 2. Set up SVC mode stack (default operating mode)
    @ -----------------------------------------------------------------------
    msr cpsr_c, #0xD3           @ SVC mode | FIQ disabled | IRQ disabled
    ldr sp, =_stack_top

    @ -----------------------------------------------------------------------
    @ 3. No VBAR on ARM926EJ-S — vectors are fixed at 0x00000000.
    @    Nothing to configure here; the linker places .vectors at 0x0.
    @ -----------------------------------------------------------------------

    @ -----------------------------------------------------------------------
    @ 4. Zero out .bss section
    @ -----------------------------------------------------------------------
    ldr r0, =_bss_start
    ldr r1, =_bss_end
    mov r2, #0
bss_zero_loop:
    cmp r0, r1
    strlt r2, [r0], #4
    blt bss_zero_loop

    @ -----------------------------------------------------------------------
    @ 5. Jump to main
    @ -----------------------------------------------------------------------
    bl main

hang:
    b hang

@ ===========================================================================
@ Fault handlers — loop forever for now
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
@ ARM926EJ-S (v5TE): LR_irq = PC+4 of interrupted instruction on IRQ entry,
@ so we subtract 4 to get the correct return address.
@ subs pc, lr, #0 atomically restores CPSR from SPSR_irq and branches to LR.
@ ===========================================================================
irq_handler:
    sub   lr, lr, #4            @ Adjust return address (IRQ offset correction)
    stmfd sp!, {r0-r12, lr}     @ Save full context onto IRQ stack

    @ -----------------------------------------------------------------------
    @ On VersatilePB the PL190 VIC does NOT auto-clear the interrupt.
    @ The C handler (timer_irq_handler) must clear the SP804 timer interrupt
    @ and then write to VIC_VECTADDR to signal end-of-interrupt to the VIC.
    @ -----------------------------------------------------------------------
    bl timer_irq_handler        @ Call C IRQ handler

    ldmfd sp!, {r0-r12, lr}     @ Restore context
    subs  pc, lr, #0            @ Return from exception: restore CPSR from SPSR

@ ===========================================================================
@ Low-level memory access
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
@ Enable IRQs — clear I bit (bit 7) in CPSR
@ ===========================================================================
.globl enable_irq
enable_irq:
    mrs r0, cpsr
    bic r0, r0, #0x80           @ clear bit 7 (I bit)
    msr cpsr_c, r0
    bx  lr

@ ===========================================================================
@ Stack and BSS allocation
@ ===========================================================================
.section .bss
.align 4

_bss_start:

@ IRQ mode stack — separate stack required since IRQ uses banked SP_irq
.align 4
_irq_stack_bottom:
    .skip 0x1000                @ 4KB IRQ stack
_irq_stack_top:

@ Main SVC stack
.align 4
_stack_bottom:
    .skip 0x2000                @ 8KB main stack
_stack_top:

_bss_end: