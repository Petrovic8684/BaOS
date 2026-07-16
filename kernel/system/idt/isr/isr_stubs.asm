extern isr_common_handler

section .text
align 4

%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    pushad
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ebx, esp
    add ebx, 48
    push ebx
    push dword 0
    push dword %1
    call isr_common_handler
    add esp, 12
    pop gs
    pop fs
    pop es
    pop ds
    popad
    iretd
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    pushad
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, [esp + 48]
    mov ebx, esp
    add ebx, 52
    push ebx
    push eax
    push dword %1
    call isr_common_handler
    add esp, 12
    pop gs
    pop fs
    pop es
    pop ds
    popad
    iretd
%endmacro

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_NOERR 9
ISR_NOERR 15
ISR_NOERR 16
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

ISR_ERR 8
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_ERR 17
