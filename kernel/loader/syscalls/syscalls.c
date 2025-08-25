#include "../../drivers/display/display.h"
#include "../../helpers/string/string.h"

enum
{
    SYS_WRITE = 0
};

#define USER_BUFFER_SIZE 256

// Kopira string iz korisničkog prostora u kernel buffer
// Pretpostavljamo da je user adresa mapirana u kernel prostor
void copy_from_user(char *kernel_buf, const char *user_buf, unsigned int max_len)
{
    unsigned int i = 0;
    for (; i < max_len - 1 && user_buf[i] != '\0'; i++)
        kernel_buf[i] = user_buf[i];
    kernel_buf[i] = '\0';
}

void handle_syscall(unsigned int syscall_id, unsigned int arg)
{
    char buffer[256];

    switch (syscall_id)
    {
    case SYS_WRITE:
        copy_from_user(buffer, (const char *)arg, sizeof(buffer));
        write(buffer);
        break;

    default:
        write("Unknown syscall!\n");
        break;
    }
}

__attribute__((naked)) void syscall_interrupt_handler()
{
    asm volatile(
        "cli\n\t" // isključi prekide
        "push %ebp\n\t"
        "mov %esp, %ebp\n\t"

        "push %eax\n\t" // syscall_id
        "push %ebx\n\t" // arg
        "call handle_syscall\n\t"
        "add $8, %esp\n\t" // očisti stack

        "pop %ebp\n\t"
        "iret");
}
