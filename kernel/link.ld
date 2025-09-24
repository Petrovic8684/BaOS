ENTRY(kernel_main)
SECTIONS
{
    . = 0x10000;
    .text : { *(.text*) }
    .data : { *(.data*) }
    .bss  : { *(.bss*) }

    . = 0x080000;
    .page_tables ALIGN(0x1000) : { *(.page_tables*) }
    
    _end = .;
    PROVIDE(end = .);
}