SECTIONS {
    . = 0x02000000;
    .text : {
        KEEP(*(.text._start))
        KEEP(*crt0.o(.text*))
        *(.text*)
    }
    .data : { *(.data*) }
    .bss  : { *(.bss*) }
    _end = .;
}
ENTRY(_start)
