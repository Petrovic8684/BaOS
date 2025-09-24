#define SYS_REBOOT 20

int main(void)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_REBOOT)
        : "eax", "memory");

    return 0;
}
