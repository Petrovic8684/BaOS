#ifndef USER_SYSCALLS_H
#define USER_SYSCALLS_H

static inline void write(const char *str)
{
    asm volatile(
        "movl %[s], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [s] "r"(str)
        : "ebx", "memory");
}

#endif
