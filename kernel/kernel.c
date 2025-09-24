#include "./system/pic/pic.h"
#include "./system/idt/idt.h"
#include "./system/gdt/gdt.h"
#include "./system/tss/tss.h"
#include "./paging/paging.h"
#include "./paging/heap/heap.h"
#include "./fs/fs.h"
#include "./loader/loader.h"
#include "./drivers/drivers.h"

__attribute__((section(".text"), used, noreturn)) void kernel_main(void)
{
    pic_init();
    idt_init();

    paging_init();
    heap_init();

    gdt_init();
    tss_init();

    fs_init();
    drivers_init();

    load_shell();

    while (1)
        ;
}
