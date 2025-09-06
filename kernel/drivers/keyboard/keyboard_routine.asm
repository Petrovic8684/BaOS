global keyboard_interrupt_service_routine
extern keyboard_interrupt_handler_wrapper

keyboard_interrupt_service_routine:
    cli
    pushad
    push ds
    push es
    mov ax, 0x10
    mov ds, ax
    mov es, ax

    call keyboard_interrupt_handler_wrapper

    pop es
    pop ds
    popad

    mov al, 0x20
    out 0x20, al

    sti         
    iret
