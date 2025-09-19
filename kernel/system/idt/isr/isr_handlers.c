#include "../../../paging/paging.h"
#include "../../../drivers/display/display.h"
#include "../../../loader/loader.h"

static int frame_is_user(unsigned int *frame_ptr)
{
    if (!frame_ptr)
        return 0;

    unsigned int maybe_cs1 = frame_ptr[1];
    if ((maybe_cs1 & 3) == 3)
        return 1;

    unsigned int maybe_cs2 = frame_ptr[3];
    if ((maybe_cs2 & 3) == 3)
        return 1;

    return 0;
}

static void panic(const char *msg, unsigned int info)
{
    fill("\033[1;41;37m");

    write("--------------------------------- FATAL ERROR ----------------------------------\n");
    write(msg);
    write("\n\n");
    write("Info: ");
    write_hex(info);
    write("\n");
    write("System halted.\n");
    write("\033[0m");

    for (;;)
        __asm__ volatile("hlt");
}

static void user_exception(const char *title, unsigned int error_code, unsigned int extra_addr)
{
    write("\033[31mERROR: ");
    write(title);
    write("\033[0m\n");

    if (extra_addr)
    {
        write("Faulting address: ");
        write_hex(extra_addr);
        write("\n");
    }

    write("Error code: ");
    write_hex(error_code);
    write("\n\n");

    write("\033[1;33mAborting user program, returning to shell...\033[0m\n");

    loader_return_eip = 0;
    loader_saved_esp = 0;
    loader_saved_ebp = 0;

    load_shell();

    for (;;)
        __asm__ volatile("hlt");
}

void isr0_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Divide by zero exception.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Divide by zero in kernel (ISR0).", error_code);
}

void isr1_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    (void)frame_ptr;
    if (frame_is_user(frame_ptr))
        user_exception("Debug exception.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Debug exception in kernel (ISR1).", error_code);
}

void isr2_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    (void)error_code;
    (void)frame_ptr;
    panic("Non-maskable interrupt (NMI).", 0);
}

void isr3_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Breakpoint.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Breakpoint in kernel (ISR3).", error_code);
}

void isr4_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Overflow exception.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Overflow in kernel (ISR4).", error_code);
}

void isr5_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("BOUND-range exceeded.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("BOUND-range exceeded in kernel (ISR5).", error_code);
}

void isr6_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Invalid opcode.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Invalid opcode in kernel (ISR6).", error_code);
}

void isr7_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Device not available (FPU).", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Device-not-available in kernel (ISR7).", error_code);
}

void isr8_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    (void)error_code;
    (void)frame_ptr;
    panic("Double fault (ISR8).", error_code);
}

void isr9_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    (void)frame_ptr;
    panic("Coprocessor segment overrun (ISR9).", error_code);
}

void isr10_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    (void)frame_ptr;
    panic("Invalid TSS (ISR10).", error_code);
}

void isr11_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Segment not present.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Segment not present in kernel (ISR11).", error_code);
}

void isr12_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Stack-segment fault.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Stack-segment fault in kernel (ISR12).", error_code);
}

void isr13_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("General protection fault.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("General protection fault in kernel (ISR13).", error_code);
}

void isr14_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    unsigned int cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));

    if (frame_is_user(frame_ptr))
        user_exception("Page fault.", error_code, cr2);
    else
        panic("Page fault in kernel (ISR14).", cr2);
}

void isr15_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    (void)frame_ptr;
    panic("Reserved exception (ISR15).", error_code);
}

void isr16_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("FPU floating point error.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("FPU floating point error in kernel (ISR16).", error_code);
}

void isr17_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Alignment check.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Alignment check in kernel (ISR17).", error_code);
}

void isr18_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    (void)frame_ptr;
    (void)error_code;
    panic("Machine check (ISR18).", error_code);
}

void isr19_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("SIMD floating-point exception.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("SIMD floating-point exception in kernel (ISR19).", error_code);
}

void isr20_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Exception ISR20.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Exception ISR20.", error_code);
}

void isr21_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Exception ISR21.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Exception ISR21.", error_code);
}

void isr22_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Exception ISR22.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Exception ISR22.", error_code);
}

void isr23_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Exception ISR23.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Exception ISR23.", error_code);
}

void isr24_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Exception ISR24.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Exception ISR24.", error_code);
}

void isr25_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Exception ISR25.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Exception ISR25.", error_code);
}

void isr26_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Exception ISR26.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Exception ISR26.", error_code);
}

void isr27_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Exception ISR27.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Exception ISR27.", error_code);
}

void isr28_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Exception ISR28.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Exception ISR28.", error_code);
}

void isr29_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Exception ISR29.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Exception ISR29.", error_code);
}

void isr30_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Exception ISR30.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Exception ISR30.", error_code);
}

void isr31_handler(unsigned int error_code, unsigned int *frame_ptr)
{
    if (frame_is_user(frame_ptr))
        user_exception("Exception ISR31.", error_code, frame_ptr ? frame_ptr[0] : 0);
    else
        panic("Exception ISR31.", error_code);
}