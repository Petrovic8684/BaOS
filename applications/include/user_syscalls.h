#ifndef USER_SYSCALLS_H
#define USER_SYSCALLS_H

enum
{
    SYS_WRITE = 0
};

static inline void write(const char *str)
{
    asm volatile(
        "movl %[arg], %%ebx\n\t"
        "movl $0, %%eax\n\t" // SYS_WRITE
        "int $0x80"
        :
        : [arg] "r"(str)
        : "eax", "ebx");
}

#endif
