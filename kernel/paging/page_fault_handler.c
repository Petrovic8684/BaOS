#include "../drivers/display/display.h"
#include "../loader/loader.h"
#include "paging.h"

void page_fault_handler_c(unsigned int cr2, unsigned int error_code)
{
    write("\033\[31m\nFATAL ERROR: Page fault.\n\033\[0m");
    write("Faulting address: ");
    write_hex(cr2);
    write("\nError code: ");
    write_hex(error_code);
    write("\n");

    if (error_code & 0x1)
        write("  - Page protection violation\n");
    else
        write("  - Page not present\n");
    if (error_code & 0x2)
        write("  - Write access\n");
    else
        write("  - Read access\n");
    if (error_code & 0x4)
        write("  - From user mode\n");
    else
        write("  - From kernel mode\n");
    if (error_code & 0x8)
        write("  - Reserved bits overwritten\n");
    if (error_code & 0x10)
        write("  - Instruction fetch\n");

    write("\033[1;33m\nAborting user program, returning to shell...\n\n\033[0m");

    loader_return_eip = 0;
    loader_saved_esp = 0;
    loader_saved_ebp = 0;

    load_user_program("shell", ((void *)0));

    for (;;)
        __asm__ volatile("hlt");
}