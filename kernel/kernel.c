#include "./fs/fs.h"
#include "./loader/idt/idt.h"
#include "./loader/loader.h"

__attribute__((section(".text"), used, noreturn)) void kernel_main(void)
{
    init_idt();
    fs_init();

    fs_change_dir("programs");
    load_user_program("bao.bin");

    while (1)
        ;
}
