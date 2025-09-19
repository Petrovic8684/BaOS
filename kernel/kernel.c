#include "./system/pic/pic.h"
#include "./system/idt/idt.h"
#include "./system/gdt/gdt.h"
#include "./system/tss/tss.h"
#include "./paging/paging.h"
#include "./fs/fs.h"
#include "./loader/loader.h"
#include "./drivers/keyboard/keyboard.h"

__attribute__((section(".text"), used, noreturn)) void kernel_main(void)
{
    pic_init();
    idt_init();

    paging_init();
    gdt_init();
    tss_init();

    fs_init();
    keyboard_init();

    load_shell();

    while (1)
        ;
}
