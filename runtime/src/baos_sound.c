#include <baos/sound.h>
#include <stdio.h>

#define SYS_BEEP 25

static inline void sys_beep(uint32_t hz, uint32_t ms)
{
    struct beep_args
    {
        uint32_t hz;
        uint32_t ms;
    } args = {hz, ms};

    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[arg], %%ebx\n\t"
        "int $0x80"
        :
        : [num] "r"(SYS_BEEP),
          [arg] "r"(&args)
        : "eax", "ebx");
}

void beep(uint32_t hz, uint32_t ms)
{
    if (hz < 20)
    {
        printf("\033[1;33mUnable to produce infrasound, clamping frequency to 20Hz.\033[0m\n");
        hz = 20;
    }
    else if (hz > 20000)
    {
        printf("\033[1;33mUnable to produce ultrasound, clamping frequency to 20kHz.\033[0m\n");
        hz = 20000;
    }

    sys_beep(hz, ms);
}