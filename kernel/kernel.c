__attribute__((section(".text"), used, noreturn)) void kernel_main(void)
{
    volatile unsigned short *video = (volatile unsigned short *)0xB8000;
    const char *message = "Hello world (from kernel)!";

    // Clear the screen
    for (int i = 0; i < 80 * 25; i++)
        video[i] = (0x07 << 8) | ' ';

    // Print message
    for (int i = 0; message[i] != 0; i++)
        video[i] = (0x07 << 8) | message[i];

    while (1)
        ;
}
