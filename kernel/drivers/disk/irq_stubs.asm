global irq14_stub
global irq15_stub

extern ata_irq_handler_c  

section .text
irq14_stub:
    pushad
    push 14            
    call ata_irq_handler_c
    add  esp, 4
    popad
    sti
    iretd

irq15_stub:
    pushad
    push 15
    call ata_irq_handler_c
    add  esp, 4
    popad
    sti
    iretd