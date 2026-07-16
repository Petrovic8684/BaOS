SECTIONS {
    . = 0x01000000;
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
