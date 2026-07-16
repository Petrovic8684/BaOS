ENTRY(kernel_main)
SECTIONS
{
    . = 0x10000;
    .text : { *(.text*) }
    .data : { *(.data*) }
    .bss  : { *(.bss*) }

    . = 0x080000;
    .segmentation_reserved ALIGN(0x1000) : { . = . + 0x80000; }

    _end = .;
    PROVIDE(end = .);
}
