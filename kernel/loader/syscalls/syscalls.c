#include "../../drivers/display/display.h"
#include "../../helpers/string/string.h"

#define USER_BUFFER_SIZE 256

void copy_from_user(char *kernel_buf, const char *user_buf, unsigned int max_len)
{
    unsigned int i = 0;
    for (; i < max_len - 1 && user_buf[i] != '\0'; i++)
        kernel_buf[i] = user_buf[i];
    kernel_buf[i] = '\0';
}

void handle_syscall(const char *user_str)
{
    char buffer[USER_BUFFER_SIZE];
    copy_from_user(buffer, user_str, sizeof(buffer));
    write(buffer);
}

__attribute__((naked)) void syscall_interrupt_handler()
{
    asm volatile(
        ".intel_syntax noprefix\n\t"
        "cli\n\t"
        "push ebp\n\t"
        "mov ebp, esp\n\t"
        "push ebx\n\t"
        "call handle_syscall\n\t"
        "add esp, 4\n\t"
        "pop ebp\n\t"
        "sti\n\t"
        "iret\n\t"
        ".att_syntax\n\t");
}
