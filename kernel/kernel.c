__attribute__((section(".text"), used, noreturn)) void kernel_main(void)
{
    extern void clear(void);
    extern void write(const char *str);
    extern void read(char *buffer, int max_len);

    char buffer[80];

    clear();

    while (1)
    {
        write("BaOS > ");
        read(buffer, 80);
        write("\n");
        write(buffer);
        write("\n\n");
    }
}
