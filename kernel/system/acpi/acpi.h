#ifndef ACPI_H
#define ACPI_H
#define ACPI_POWER_OFF 5

#include "../../helpers/ports/ports.h"
#include "../../drivers/display/display.h"

typedef struct
{
    char signature[8];
    unsigned char checksum;
    char oemid[6];
    unsigned char revision;
    unsigned long rsdt_address;
} __attribute__((packed)) acpi_rsdp_t;

typedef struct
{
    char signature[4];
    unsigned long length;
    unsigned char revision;
    unsigned char checksum;
    char oemid[6];
    char oem_table_id[8];
    unsigned long oem_revision;
    unsigned long creator_id;
    unsigned long creator_revision;
    unsigned long entries[];
} __attribute__((packed)) acpi_rsdt_t;

typedef struct
{
    char signature[4];
    unsigned long length;
    unsigned char revision;
    unsigned char checksum;
    char oemid[6];
    char oem_table_id[8];
    unsigned long oem_revision;
    unsigned long creator_id;
    unsigned long creator_revision;
    unsigned long firmware_ctrl;
    unsigned long dsdt;
    unsigned char reserved;
    unsigned char preferred_pm_profile;
    unsigned short sci_int;
    unsigned long smi_cmd;
    unsigned char acpi_enable;
    unsigned char acpi_disable;
    unsigned char s4bios_req;
    unsigned char pstate_cnt;
    unsigned long pm1a_event_block;
    unsigned long pm1b_event_block;
    unsigned long pm1a_control_block;
    unsigned long pm1b_control_block;
} __attribute__((packed)) acpi_fadt_t;

acpi_rsdp_t *find_rsdp(void);
acpi_fadt_t *find_fadt(acpi_rsdt_t *rsdt);

#endif