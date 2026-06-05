ENTRY(_start)

STACK_SIZE = 8K;

MEMORY
{
    ram : ORIGIN = 0x82000000, LENGTH = 256K
}

SECTIONS
{
    . = ORIGIN(ram);

    .text :
    {
        *(.text*)
        *(.rodata*)
    } > ram

    .data : AT(ADDR(.text) + SIZEOF(.text))
    {
        __data_start__ = .;
        *(.data*)
        __data_end__ = .;
    } > ram

    .bss :
    {
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        __bss_end__ = .;
    } > ram

    /* Stacks placed at the top of RAM, growing downward.
     * IRQ stack sits below the SVC stack.
     * These symbols are used by reset_handler in root.s.         */
    _stack_top     = ORIGIN(ram) + LENGTH(ram);       /* 0x82040000 */
    _stack_bottom  = _stack_top  - 0x2000;            /* SVC: 8KB   */
    _irq_stack_top = _stack_bottom;
    _irq_stack_bottom = _irq_stack_top - 0x4000;      /* IRQ: 16KB  */

    ASSERT(__bss_end__ <= _irq_stack_bottom, "BSS overlaps IRQ stack")
}