#include "acpi.h"

acpi_rsdp_t *find_rsdp(void)
{
    unsigned long addr;
    for (addr = 0xE0000; addr < 0x100000; addr += 16)
    {
        acpi_rsdp_t *rsdp = (acpi_rsdp_t *)addr;
        if (rsdp->signature[0] == 'R' && rsdp->signature[1] == 'S' &&
            rsdp->signature[2] == 'D' && rsdp->signature[3] == ' ' &&
            rsdp->signature[4] == 'P' && rsdp->signature[5] == 'T' &&
            rsdp->signature[6] == 'R' && rsdp->signature[7] == ' ')
        {
            return rsdp;
        }
    }
    return 0;
}

acpi_fadt_t *find_fadt(acpi_rsdt_t *rsdt)
{
    unsigned long count = (rsdt->length - sizeof(acpi_rsdt_t)) / 4;
    unsigned long i;
    for (i = 0; i < count; i++)
    {
        acpi_fadt_t *table = (acpi_fadt_t *)rsdt->entries[i];
        if (table->signature[0] == 'F' && table->signature[1] == 'A' &&
            table->signature[2] == 'C' && table->signature[3] == 'P')
        {
            return table;
        }
    }
    return 0;
}