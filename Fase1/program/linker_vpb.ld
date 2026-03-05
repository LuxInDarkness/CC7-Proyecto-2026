ENTRY(_start)

MEMORY
{
    ram (rwx) : ORIGIN = 0x00000000, LENGTH = 64M
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

    _stack_bottom = __bss_end__;
    _stack_top = ORIGIN(ram) + LENGTH(ram);
}