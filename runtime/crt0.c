__attribute__((naked)) void _start(void)
{
    __asm__ volatile(
        "movl (%esp), %eax\n\t"
        "movl 4(%esp), %ebx\n\t"
        "pushl %ebx\n\t"
        "pushl %eax\n\t"
        "call main\n\t"
        "movl %eax, %ebx\n\t"
        "movl $0, %eax\n\t"
        "int $0x80\n\t");
}
