#include "acpi.h"
#include "../../helpers/ports/ports.h"
#include "../../drivers/display/display.h"
#include "../../drivers/speaker/melodies/melodies.h"
#include "../../paging/paging.h"

#define KERNEL_PHYS_TO_VIRT(addr) ((void *)((unsigned long)(addr)))

static unsigned long get_ebda_base(void)
{
    ensure_phys_range_mapped(0x040E, 2);
    unsigned short *p = (unsigned short *)KERNEL_PHYS_TO_VIRT(0x040E);
    return ((unsigned long)(*p)) << 4;
}

static acpi_rsdp_t *find_rsdp(void)
{
    unsigned long addr;
    unsigned long ebda = get_ebda_base();
    if (ebda && ebda < 0xA0000)
    {
        for (addr = ebda; addr < ebda + 0x400; addr += 16)
        {
            ensure_phys_range_mapped(addr, sizeof(acpi_rsdp_t));
            acpi_rsdp_t *rsdp = (acpi_rsdp_t *)KERNEL_PHYS_TO_VIRT(addr);
            if (rsdp->signature[0] == 'R' && rsdp->signature[1] == 'S' &&
                rsdp->signature[2] == 'D' && rsdp->signature[3] == ' ' &&
                rsdp->signature[4] == 'P' && rsdp->signature[5] == 'T' &&
                rsdp->signature[6] == 'R' && rsdp->signature[7] == ' ')
                return rsdp;
        }
    }

    for (addr = 0xE0000UL; addr < 0x100000UL; addr += 16)
    {
        ensure_phys_range_mapped(addr, sizeof(acpi_rsdp_t));
        acpi_rsdp_t *rsdp = (acpi_rsdp_t *)KERNEL_PHYS_TO_VIRT(addr);
        if (rsdp->signature[0] == 'R' && rsdp->signature[1] == 'S' &&
            rsdp->signature[2] == 'D' && rsdp->signature[3] == ' ' &&
            rsdp->signature[4] == 'P' && rsdp->signature[5] == 'T' &&
            rsdp->signature[6] == 'R' && rsdp->signature[7] == ' ')
            return rsdp;
    }
    return 0;
}

static acpi_fadt_t *find_fadt_from_sdt(acpi_sdt_header_t *sdt)
{
    if (!sdt)
        return 0;

    unsigned long entries_bytes = sdt->length - sizeof(acpi_sdt_header_t);
    if (entries_bytes == 0)
        return 0;

    int is_xsdt = (sdt->signature[0] == 'X' && sdt->signature[1] == 'S' &&
                   sdt->signature[2] == 'D' && sdt->signature[3] == 'T');

    unsigned long count = entries_bytes / (is_xsdt ? 8 : 4);
    unsigned long i;
    for (i = 0; i < count; i++)
    {
        unsigned long entry_phys = 0;
        if (is_xsdt)
        {
            unsigned long long *entries64 = (unsigned long long *)((char *)sdt + sizeof(acpi_sdt_header_t));
            entry_phys = (unsigned long)entries64[i];
        }
        else
        {
            unsigned int *entries32 = (unsigned int *)((char *)sdt + sizeof(acpi_sdt_header_t));
            entry_phys = (unsigned long)entries32[i];
        }

        if (entry_phys == 0)
            continue;

        ensure_phys_range_mapped(entry_phys, 0x1000);
        acpi_sdt_header_t *entry = (acpi_sdt_header_t *)KERNEL_PHYS_TO_VIRT(entry_phys);
        if (entry->signature[0] == 'F' && entry->signature[1] == 'A' &&
            entry->signature[2] == 'C' && entry->signature[3] == 'P')
        {
            return (acpi_fadt_t *)entry;
        }
    }
    return 0;
}

static acpi_fadt_t *find_fadt(acpi_rsdt_t *rsdt)
{
    if (!rsdt)
        return 0;
    return find_fadt_from_sdt((acpi_sdt_header_t *)rsdt);
}

static int parse_s5_sleep_type(acpi_fadt_t *fadt, unsigned short *slp_typa, unsigned short *slp_typb)
{
    if (!fadt || !slp_typa || !slp_typb)
        return 0;

    unsigned long dsdt_phys = (unsigned long)fadt->dsdt;
    if (fadt->header.revision >= 2 && fadt->x_dsdt != 0)
        dsdt_phys = (unsigned long)fadt->x_dsdt;

    if (dsdt_phys == 0)
        return 0;

    ensure_phys_range_mapped(dsdt_phys, sizeof(acpi_sdt_header_t));
    acpi_sdt_header_t *dsdt_hdr = (acpi_sdt_header_t *)KERNEL_PHYS_TO_VIRT(dsdt_phys);
    if (dsdt_hdr->length == 0)
        return 0;

    ensure_phys_range_mapped(dsdt_phys, dsdt_hdr->length);
    unsigned char *dsdt = (unsigned char *)dsdt_hdr;
    unsigned long len = dsdt_hdr->length;

    unsigned long i;
    for (i = sizeof(acpi_sdt_header_t); i < len - 4; i++)
    {
        if (dsdt[i] == '_' && dsdt[i + 1] == 'S' && dsdt[i + 2] == '5' && dsdt[i + 3] == '_')
        {
            unsigned char *p = dsdt + i + 4;
            unsigned char *end = dsdt + len;

            unsigned char *pkg = p;
            int steps = 0;
            while (pkg < end && steps < 40)
            {
                if (*pkg == 0x12)
                    break;
                pkg++;
                steps++;
            }
            if (pkg >= end || *pkg != 0x12)
                continue;

            unsigned char *val = pkg + 1;
            int pkgl = 0;
            while (val < end)
            {
                unsigned char b = *val;
                val++;
                pkgl++;
                if ((b & 0x80) == 0)
                    break;
                if (pkgl > 4)
                    break;
            }

            unsigned short found[2] = {0xFFFF, 0xFFFF};
            int found_count = 0;
            unsigned char *q = val;
            while (q < end && q < val + 20 && found_count < 2)
            {
                if (*q == 0x0A)
                {
                    q++;
                    if (q < end)
                        found[found_count++] = (unsigned short)(*q++);
                    continue;
                }
                if ((*q & 0x80) == 0)
                    found[found_count++] = (unsigned short)(*q++);
                else
                    q++;
            }

            if (found_count >= 1)
            {
                *slp_typa = found[0];
                *slp_typb = (found_count >= 2) ? found[1] : found[0];
                return 1;
            }
        }
    }

    return 0;
}

static void enable_acpi_if_needed(acpi_fadt_t *fadt)
{
    if (!fadt)
        return;

    unsigned int pm1a_cnt = fadt->pm1a_control_block;
    if (pm1a_cnt == 0)
        return;

    unsigned short cnt = 0;
    if (pm1a_cnt < 0x10000u)
        cnt = inw((unsigned short)pm1a_cnt);
    else
    {
        ensure_phys_range_mapped(pm1a_cnt, 2);
        volatile unsigned short *mmio = (volatile unsigned short *)KERNEL_PHYS_TO_VIRT(pm1a_cnt);
        cnt = *mmio;
    }

    if ((cnt & 1) == 0 && fadt->smi_command != 0 && fadt->acpi_enable != 0)
        outb((unsigned short)fadt->smi_command, (unsigned char)fadt->acpi_enable);
}

void power_off(void)
{
    write("Shutting down...\n");
    play_shutdown_melody();

    ensure_phys_range_mapped(0xE0000u, 0x20000u);

    acpi_rsdp_t *rsdp = find_rsdp();
    acpi_fadt_t *fadt = 0;

    if (rsdp)
    {
        if (rsdp->rsdt_address != 0)
        {
            ensure_phys_range_mapped(rsdp->rsdt_address, 0x1000);
            acpi_rsdt_t *rsdt = (acpi_rsdt_t *)KERNEL_PHYS_TO_VIRT(rsdp->rsdt_address);
            if (rsdt && rsdt->header.signature[0] == 'R' && rsdt->header.signature[1] == 'S' &&
                rsdt->header.signature[2] == 'D' && rsdt->header.signature[3] == 'T')
            {
                if (rsdt->header.length > 0)
                    ensure_phys_range_mapped(rsdp->rsdt_address, rsdt->header.length);
                fadt = find_fadt(rsdt);
            }
        }

        if (!fadt && rsdp->revision >= 2 && rsdp->xsdt_address != 0)
        {
            unsigned long long xsdt_phys = rsdp->xsdt_address;
            ensure_phys_range_mapped((unsigned long)xsdt_phys, sizeof(acpi_sdt_header_t));
            acpi_sdt_header_t *xsdt_hdr = (acpi_sdt_header_t *)KERNEL_PHYS_TO_VIRT((unsigned long)xsdt_phys);
            if (xsdt_hdr && xsdt_hdr->signature[0] == 'X' && xsdt_hdr->signature[1] == 'S' &&
                xsdt_hdr->signature[2] == 'D' && xsdt_hdr->signature[3] == 'T')
            {
                if (xsdt_hdr->length > 0)
                    ensure_phys_range_mapped((unsigned long)xsdt_phys, xsdt_hdr->length);
                fadt = find_fadt_from_sdt(xsdt_hdr);
            }
        }
    }

    unsigned short slp_typa = 0, slp_typb = 0;
    if (fadt)
    {
        if (!parse_s5_sleep_type(fadt, &slp_typa, &slp_typb))
            slp_typa = slp_typb = 0x1;

        enable_acpi_if_needed(fadt);

        if (fadt->pm1a_control_block != 0)
        {
            unsigned int pm1 = fadt->pm1a_control_block;
            unsigned short value = (1 << 13) | (slp_typa << 10);
            if (pm1 < 0x10000u)
                outw((unsigned short)pm1, value);
            else
            {
                ensure_phys_range_mapped(pm1, 2);
                volatile unsigned short *mmio = (volatile unsigned short *)KERNEL_PHYS_TO_VIRT(pm1);
                *mmio = value;
            }

            if (fadt->pm1b_control_block != 0)
            {
                unsigned int pm1b = fadt->pm1b_control_block;
                unsigned short valueb = (1 << 13) | (slp_typb << 10);
                if (pm1b < 0x10000u)
                    outw((unsigned short)pm1b, valueb);
                else
                {
                    ensure_phys_range_mapped(pm1b, 2);
                    volatile unsigned short *mmio_b = (volatile unsigned short *)
                        KERNEL_PHYS_TO_VIRT(pm1b);
                    *mmio_b = valueb;
                }
            }

            for (volatile unsigned long i = 0; i < 1000000UL; ++i)
                __asm__ volatile("nop");
        }
    }

    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);

    for (volatile unsigned long i = 0; i < 1000000UL; ++i)
        __asm__ volatile("nop");

    write("\n\033[31mError: ACPI poweroff failed. Halting...\033[0m\n");
    for (;;)
        __asm__ volatile("hlt");
}

static int acpi_reset_via_fadt(acpi_fadt_t *fadt)
{
    if (!fadt)
        return 0;

#ifdef FADT_HAS_RESET_REG
    if (fadt->header.revision >= 1 && fadt->reset_reg.address != 0)
    {
        unsigned char space_id = fadt->reset_reg.address_space_id;
        unsigned long long addr = fadt->reset_reg.address;
        unsigned char value = fadt->reset_value;

        if (space_id == 1)
        {
            outb((unsigned short)addr, value);
            return 1;
        }
        else if (space_id == 0)
        {
            ensure_phys_range_mapped((unsigned long)addr, 1);
            volatile unsigned char *mm = (volatile unsigned char *)KERNEL_PHYS_TO_VIRT((unsigned long)addr);
            *mm = value;
            return 1;
        }
    }
#endif
    return 0;
}

void reboot(void)
{
    write("Rebooting...\n");
    play_restart_melody();

    __asm__ volatile("cli");

    acpi_rsdp_t *rsdp = find_rsdp();
    acpi_fadt_t *fadt = 0;

    if (rsdp)
    {
        if (rsdp->rsdt_address != 0)
        {
            ensure_phys_range_mapped(rsdp->rsdt_address, 0x1000);
            acpi_rsdt_t *rsdt = (acpi_rsdt_t *)KERNEL_PHYS_TO_VIRT(rsdp->rsdt_address);
            if (rsdt && rsdt->header.signature[0] == 'R' &&
                rsdt->header.signature[1] == 'S' && rsdt->header.signature[2] == 'D' &&
                rsdt->header.signature[3] == 'T')
            {
                if (rsdt->header.length > 0)
                    ensure_phys_range_mapped(rsdp->rsdt_address, rsdt->header.length);
                fadt = find_fadt(rsdt);
            }
        }
        if (!fadt && rsdp->revision >= 2 && rsdp->xsdt_address != 0)
        {
            unsigned long long xsdt_phys = rsdp->xsdt_address;
            ensure_phys_range_mapped((unsigned long)xsdt_phys, sizeof(acpi_sdt_header_t));
            acpi_sdt_header_t *xsdt_hdr = (acpi_sdt_header_t *)KERNEL_PHYS_TO_VIRT((unsigned long)xsdt_phys);
            if (xsdt_hdr && xsdt_hdr->signature[0] == 'X' &&
                xsdt_hdr->signature[1] == 'S' && xsdt_hdr->signature[2] == 'D' &&
                xsdt_hdr->signature[3] == 'T')
            {
                if (xsdt_hdr->length > 0)
                    ensure_phys_range_mapped((unsigned long)xsdt_phys, xsdt_hdr->length);
                fadt = find_fadt_from_sdt(xsdt_hdr);
            }
        }
    }

    if (fadt)
        if (acpi_reset_via_fadt(fadt))
            for (volatile unsigned int i = 0; i < 1000000; ++i)
                __asm__ volatile("nop");

    unsigned char val = inb(0xCF9);
    val |= 0x02;
    outb(0xCF9, val);

    for (volatile unsigned int i = 0; i < 100000; ++i)
        __asm__ volatile("nop");

    for (int i = 0; i < 100000; ++i)
    {
        unsigned char st = inb(0x64);
        if ((st & 0x02) == 0)
            break;
    }
    outb(0xCF9, 0x0E);

    for (volatile unsigned int i = 0; i < 1000000; ++i)
        __asm__ volatile("nop");

    write("ACPI Reboot failed. Forcing triple fault.\n");
    struct
    {
        unsigned short limit;
        unsigned int base;
    } __attribute__((packed)) idt_null = {0, 0};

    __asm__ volatile("lidt %0" ::"m"(idt_null));
    __asm__ volatile("ud2");

    for (;;)
        __asm__ volatile("hlt");
}