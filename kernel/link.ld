ENTRY(kernel_main)
SECTIONS
{
    . = 0x10000;
    .text : { *(.text*) }
    .data : { *(.data*) }
    .bss  : { *(.bss*) }
    _end = .;
    PROVIDE(end = .);
}