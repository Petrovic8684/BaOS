#include "./system/idt/idt.h"
#include "./system/gdt/gdt.h"
#include "./system/tss/tss.h"
#include "./paging/paging.h"
#include "./fs/fs.h"
#include "./loader/loader.h"

__attribute__((section(".text"), used, noreturn)) void kernel_main(void)
{
    idt_init();

    paging_init();
    gdt_init();
    tss_init();

    fs_init();

    load_user_program("/programs/shell", ((void *)0));

    while (1)
        ;
}
