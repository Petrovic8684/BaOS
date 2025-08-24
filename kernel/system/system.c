#include "system.h"

void power_off(void)
{
    acpi_rsdp_t *rsdp = find_rsdp();
    if (!rsdp)
        return;

    acpi_rsdt_t *rsdt = (acpi_rsdt_t *)rsdp->rsdt_address;
    acpi_fadt_t *fadt = find_fadt(rsdt);
    if (!fadt)
        return;

    unsigned short port = (unsigned short)fadt->pm1a_control_block;
    if (!port)
        return;

    outw(port, (1 << 13) | (ACPI_POWER_OFF << 10));
    while (1)
        __asm__("hlt");
}

const char *os_name(void)
{
    return OS_NAME;
}

const char *kernel_version(void)
{
    return KERNEL_VERSION;
}
