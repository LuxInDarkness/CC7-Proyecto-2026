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
    @ 2. Set up ABT mode stack
    @    Switch to ABT mode (0x17), disable IRQ+FIQ, set ABT stack pointer.
    @    The abort handlers push 14 registers (56 bytes) on this stack.
    @ -----------------------------------------------------------------------
    msr cpsr_c, #0xD7           @ ABT mode | FIQ disabled | IRQ disabled
    ldr sp, =_abt_stack_top

    @ -----------------------------------------------------------------------
    @ 3. Set up SVC mode stack (default operating mode)
    @ -----------------------------------------------------------------------
    msr cpsr_c, #0xD3           @ SVC mode | FIQ disabled | IRQ disabled
    ldr sp, =_stack_top

    @ -----------------------------------------------------------------------
    @ 4. No VBAR on ARM926EJ-S — vectors are fixed at 0x00000000.
    @    Nothing to configure here; the linker places .vectors at 0x0.
    @ -----------------------------------------------------------------------

    @ -----------------------------------------------------------------------
    @ 5. Zero out .bss section
    @ -----------------------------------------------------------------------
    ldr r0, =_bss_start
    ldr r1, =_bss_end
    mov r2, #0
bss_zero_loop:
    cmp r0, r1
    strlt r2, [r0], #4
    blt bss_zero_loop

    @ -----------------------------------------------------------------------
    @ 6. Jump to main
    @ -----------------------------------------------------------------------
    bl main

.globl hang
hang:
    b hang

@ ===========================================================================
@ Fault handlers — loop forever for now
@ ===========================================================================
undefined_handler:
    b hang

@ ===========================================================================
@ SWI Handler
@
@ lr = address of instruction after the SWI (correct return address)
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
    ldr   r0, =next_spsr         @ Load address of global next_spsr
    ldr   r0, [r0]               @ Load next_spsr value
    msr   spsr_cxsf, r0          @ Override SPSR_svc with next process's SPSR
    movs  pc, lr                @ exception return, jump to next process

@ ===========================================================================
@ Prefetch Abort Handler
@
@ ARM926EJ-S abort entry: CPU switches to ABT mode, LR_abt = fault addr + 4.
@ Saves user context on ABT stack, reads IFSR/IFAR from CP15, calls the C
@ fault handler. The C handler classifies the fault, terminates the active
@ process, and switches to the next ready process.
@
@ Same CP15 registers as ARMv7-A for FSR/FAR — ARMv5 shares this mapping.
@ ===========================================================================
prefetch_handler:
    sub   lr, lr, #4            @ LR_abt = fault address + 4, adjust to fault addr
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
    ldr   r0, =next_spsr         @ Load address of global next_spsr
    ldr   r0, [r0]               @ Load next_spsr value
    msr   spsr_cxsf, r0          @ Override SPSR_abt with next process's SPSR
    subs  pc, lr, #0            @ Exception return — restore CPSR from SPSR_abt

@ ===========================================================================
@ Data Abort Handler
@
@ ARM926EJ-S data abort: LR_abt = fault address + 8, subtract 8 for actual
@ fault address. Reads DFSR/DFAR from CP15 and calls fault_c_handler().
@ ===========================================================================
data_handler:
    sub   lr, lr, #8            @ LR_abt = fault address + 8, adjust to fault addr
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
    ldr   r0, =next_spsr         @ Load address of global next_spsr
    ldr   r0, [r0]               @ Load next_spsr value
    msr   spsr_cxsf, r0          @ Override SPSR_abt with next process's SPSR
    subs  pc, lr, #0            @ Exception return — restore CPSR from SPSR_abt

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

    mov   r0, sp                @ pass IRQ stack pointer as argument to C
    bl timer_irq_handler        @ Call C IRQ handler

    ldmfd sp!, {r0-r12, lr}     @ Restore context
    ldr   r0, =next_spsr         @ Load address of global next_spsr
    ldr   r0, [r0]               @ Load next_spsr value
    msr   spsr_cxsf, r0          @ Override SPSR_irq with process's SPSR
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
@ enter_first_task — explicit kernel-to-user transition (Spec Section 3.4)
@
@ r0 = user stack pointer  (SP_usr)
@ r1 = user program counter (entry point)
@ r2 = target SPSR          (0x10 = USR mode, IRQ enabled)
@
@ Constructs the first user context and does an exception-style return
@ into USR mode.  Never returns to the caller.
@ ===========================================================================
.globl enter_first_task
enter_first_task:
    msr   spsr_cxsf, r2          @ SPSR_svc = target SPSR (USR mode)

    @ Write SP_usr by temporarily switching to USR mode
    mrs   r3, cpsr
    orr   r4, r3, #0xC0          @ disable IRQ+FIQ during the switch
    bic   r4, r4, #0x1F
    orr   r4, r4, #0x10          @ USR mode
    msr   cpsr_c, r4
    mov   sp, r0                  @ SP_usr = user stack
    msr   cpsr_c, r3              @ back to SVC mode

    @ Set LR_svc for the exception return
    mov   lr, r1

    @ Zero out user-visible registers (r0-r12)
    mov   r0, #0
    mov   r1, #0
    mov   r2, #0
    mov   r3, #0
    mov   r4, #0
    mov   r5, #0
    mov   r6, #0
    mov   r7, #0
    mov   r8, #0
    mov   r9, #0
    mov   r10, #0
    mov   r11, #0
    mov   r12, #0

    @ Exception return: CPSR <- SPSR_svc, PC <- LR_svc
    @ CPU enters USR mode with IRQs enabled (per SPSR bit 7 = 0)
    movs  pc, lr

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
    .skip 0x4000                @ 16KB IRQ stack
_irq_stack_top:

@ ABT mode stack — used by prefetch and data abort handlers
.align 4
_abt_stack_bottom:
    .skip 0x400                 @ 1KB abort stack
_abt_stack_top:

@ Main SVC stack
.align 4
_stack_bottom:
    .skip 0x2000                @ 8KB main stack
_stack_top:

_bss_end: