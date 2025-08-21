#include "./fs/fs.h"
#include "../shell/shell.h"

__attribute__((section(".text"), used, noreturn)) void kernel_main(void)
{
    fs_init();
    shell_main();

    while (1)
        ;
}
