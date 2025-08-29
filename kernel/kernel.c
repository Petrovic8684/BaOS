#include "./system/idt/idt.h"
#include "./paging/paging.h"
#include "./system/gdt/gdt.h"
#include "./system/tss/tss.h"
#include "./fs/fs.h"
#include "./loader/loader.h"

__attribute__((section(".text"), used, noreturn)) void kernel_main(void)
{
    idt_init();

    paging_install();
    gdt_init();
    tss_init();

    fs_init();

    fs_change_dir("programs");
    load_user_program("shell", ((void *)0));

    while (1)
        ;
}
