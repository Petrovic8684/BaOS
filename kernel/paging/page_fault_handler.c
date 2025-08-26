#include "../drivers/display/display.h"
#include "paging.h"

void page_fault_handler_c(uint32_t cr2, uint32_t error_code)
{
    write("Page fault!\nFaulting address: ");
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

    for (;;)
        ;
}