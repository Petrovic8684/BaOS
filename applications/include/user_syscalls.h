#ifndef USER_SYSCALLS_H
#define USER_SYSCALLS_H

#define SYS_EXIT 0
#define SYS_WRITE 1
#define SYS_CLEAR 2

static inline void exit(int code)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[c], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_EXIT), [c] "r"(code)
        : "eax", "ebx", "memory");
}

static inline void write(const char *str)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[s], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_WRITE), [s] "r"(str)
        : "eax", "ebx", "memory");
}

static inline void clear()
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_CLEAR)
        : "eax", "memory");
}

#endif
