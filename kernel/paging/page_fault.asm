global page_fault_handler
extern page_fault_handler_c
extern write

section .text
align 4

page_fault_handler:
    cli
    pusha 
    mov eax, [esp + 32]
    mov ebx, cr2

    push eax 
    push ebx
    call page_fault_handler_c
    add esp, 8 
    popa
    sti
    iret
