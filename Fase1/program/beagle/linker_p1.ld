ENTRY(_start)

STACK_SIZE = 8K;

MEMORY
{
    ram : ORIGIN = 0x82100000, LENGTH = 64K
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

    _stack_top = ORIGIN(ram) + LENGTH(ram);
    _stack_bottom = _stack_top - STACK_SIZE;

    ASSERT(__bss_end__ <= _stack_bottom, "P1 image overlaps reserved stack region")
}
