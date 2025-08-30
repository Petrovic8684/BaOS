#include "acpi.h"

acpi_rsdp_t *find_rsdp(void)
{
    unsigned long addr;
    for (addr = 0xE0000UL; addr < 0x100000UL; addr += 16)
    {
        acpi_rsdp_t *rsdp = (acpi_rsdp_t *)KERNEL_PHYS_TO_VIRT(addr);
        if (rsdp->signature[0] == 'R' && rsdp->signature[1] == 'S' &&
            rsdp->signature[2] == 'D' && rsdp->signature[3] == ' ' &&
            rsdp->signature[4] == 'P' && rsdp->signature[5] == 'T' &&
            rsdp->signature[6] == 'R' && rsdp->signature[7] == ' ')
            return rsdp;
    }
    return 0;
}

acpi_fadt_t *find_fadt(acpi_rsdt_t *rsdt)
{
    if (!rsdt)
        return 0;

    unsigned long entries_bytes = rsdt->header.length - (unsigned int)sizeof(acpi_sdt_header_t);
    if (entries_bytes == 0)
        return 0;

    unsigned long count = entries_bytes / 4;
    unsigned long i;
    for (i = 0; i < count; i++)
    {
        unsigned int entry_phys = rsdt->entries[i];
        if (entry_phys == 0)
            continue;

        ensure_phys_range_mapped(entry_phys, 0x1000);

        acpi_fadt_t *table = (acpi_fadt_t *)KERNEL_PHYS_TO_VIRT(entry_phys);
        if (table->header.signature[0] == 'F' && table->header.signature[1] == 'A' &&
            table->header.signature[2] == 'C' && table->header.signature[3] == 'P')
            return table;
    }
    return 0;
}

void power_off(void)
{
    write("Shutting down...\n");

    ensure_phys_range_mapped(0xE0000u, 0x20000u);

    acpi_rsdp_t *rsdp = find_rsdp();
    if (!rsdp)
    {
        write_colored("Error: ACPI RSDP not found.\n", 0x04);
        return;
    }

    if (rsdp->rsdt_address == 0)
    {
        write_colored("Error: RSDT address == 0 (maybe XSDT/ACPIv2). Cannot power off.\n", 0x04);
        return;
    }

    ensure_phys_range_mapped(rsdp->rsdt_address, 0x1000);
    acpi_rsdt_t *rsdt = (acpi_rsdt_t *)KERNEL_PHYS_TO_VIRT(rsdp->rsdt_address);
    if (rsdt == 0)
    {
        write_colored("Error: RSDT pointer null after conversion.\n", 0x04);
        return;
    }

    if (rsdt->header.signature[0] != 'R' || rsdt->header.signature[1] != 'S' ||
        rsdt->header.signature[2] != 'D' || rsdt->header.signature[3] != 'T')
    {
        write_colored("Error: RSDT signature mismatch.\n", 0x04);
        return;
    }

    if (rsdt->header.length > 0)
        ensure_phys_range_mapped(rsdp->rsdt_address, rsdt->header.length);

    acpi_fadt_t *fadt = find_fadt(rsdt);
    if (!fadt)
    {
        write_colored("Error: FADT not found in RSDT.\n", 0x04);

        return;
    }

    unsigned int pm1 = fadt->pm1a_control_block;
    if (pm1 == 0)
    {
        write_colored("Error: pm1a_control_block == 0.\n", 0x04);
        return;
    }

    unsigned short value = (1 << 13) | (ACPI_POWER_OFF << 10);

    if (pm1 < 0x10000u)
    {
        unsigned short port = (unsigned short)(pm1 & 0xFFFFu);
        outw(port, value);
    }
    else
    {
        ensure_phys_range_mapped(pm1, 2);
        volatile unsigned short *mmio = (volatile unsigned short *)KERNEL_PHYS_TO_VIRT(pm1);
        *mmio = value;
    }

    for (;;)
        __asm__ volatile("hlt");
}