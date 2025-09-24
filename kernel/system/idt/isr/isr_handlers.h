#ifndef ISR_HANDLERS_H
#define ISR_HANDLERS_H

typedef struct regs
{
    unsigned int ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no;
    unsigned int err_code;
    unsigned int eip, cs, eflags, useresp, ss;
} regs_t;

void isr0_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr1_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr2_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr3_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr4_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr5_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr6_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr7_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr8_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr9_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr10_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr11_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr12_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr13_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr14_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr15_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr16_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr17_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr18_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr19_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr20_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr21_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr22_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr23_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr24_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr25_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr26_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr27_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr28_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr29_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr30_handler(unsigned int error_code, unsigned int *frame_ptr);
void isr31_handler(unsigned int error_code, unsigned int *frame_ptr);

#endif