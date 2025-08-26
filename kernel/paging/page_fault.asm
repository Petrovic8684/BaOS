global page_fault_handler
extern page_fault_handler_c
extern write

section .text
align 4

page_fault_handler:
    cli
    pusha
    mov eax, cr2
    push eax

    mov eax, [esp + 36]
    push eax

    call page_fault_handler_c
    add esp, 8

    popa

.hang:
    hlt
    jmp .hang
