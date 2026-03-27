ENTRY(_start)

STACK_SIZE = 8K;

MEMORY
{
    ram (rwx) : ORIGIN = 0x00000000, LENGTH = 64K
}

SECTIONS
{
    . = ORIGIN(ram);

    .text :
    {
        KEEP(*(.vectors))
        *(.text*)
        *(.rodata*)
    } > ram

    .data :
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

    _stack_top = ORIGIN(ram) + LENGTH(ram);
    _stack_bottom = _stack_top - STACK_SIZE;

    ASSERT(__bss_end__ <= _stack_bottom, "OS image overlaps reserved stack region")
}