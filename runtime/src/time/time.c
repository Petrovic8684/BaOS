#include <time.h>
#include "internal/syscalls.h"

time_t time(time_t *t)
{
    time_t secs;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[secs]\n\t"
        : [secs] "=r"(secs)
        : [num] "i"(SYS_RTC_NOW)
        : "eax", "ebx", "memory");

    if (t)
        *t = secs;
    return secs;
}
