.section .text
.syntax unified
.code 32
.globl _start

_start:
    ldr sp, =_stack_top

    ldr r0, =__bss_start__
    ldr r1, =__bss_end__
    mov r2, #0

bss_zero_loop:
    cmp r0, r1
    strlt r2, [r0], #4
    blt bss_zero_loop

    bl main

hang:
    b hang
