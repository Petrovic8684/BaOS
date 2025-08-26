#include "./fs/fs.h"
#include "./loader/idt/idt.h"
#include "./paging/paging.h"
#include "./drivers/display/display.h"

#include "./loader/loader.h"

__attribute__((section(".text"), used, noreturn)) void kernel_main(void)
{
    idt_init();

    paging_install();
    setup_kernel_gdt_and_tss();

    fs_init();

    fs_change_dir("programs");
    load_user_program("bao.bin");

    while (1)
        ;
}
