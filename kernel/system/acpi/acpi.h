#ifndef ACPI_H
#define ACPI_H

#define ACPI_POWER_OFF 5

typedef struct
{
    char signature[8];
    unsigned char checksum;
    char oemid[6];
    unsigned char revision;
    unsigned int rsdt_address;
} __attribute__((packed)) acpi_rsdp_t;

typedef struct
{
    char signature[4];
    unsigned int length;
    unsigned char revision;
    unsigned char checksum;
    char oemid[6];
    char oem_table_id[8];
    unsigned int oem_revision;
    unsigned int creator_id;
    unsigned int creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;

typedef struct
{
    acpi_sdt_header_t header;
    unsigned int entries[];
} __attribute__((packed)) acpi_rsdt_t;

typedef struct
{
    acpi_sdt_header_t header;
    unsigned int firmware_ctrl;
    unsigned int dsdt;
    unsigned char reserved;
    unsigned char preferred_power_management_profile;
    unsigned short sci_interrupt;
    unsigned int smi_command_port;
    unsigned char acpi_enable;
    unsigned char acpi_disable;
    unsigned char s4bios_req;
    unsigned char pstate_control;
    unsigned int pm1a_event_block;
    unsigned int pm1b_event_block;
    unsigned int pm1a_control_block;
    unsigned int pm1b_control_block;
    unsigned int pm2_control_block;
    unsigned int pm_timer_block;
    unsigned int gpe0_block;
    unsigned int gpe1_block;
    unsigned char pm1_event_length;
    unsigned char pm1_control_length;
    unsigned char pm2_control_length;
    unsigned char pm_timer_length;
    unsigned char gpe0_length;
    unsigned char gpe1_length;
    unsigned char gpe1_base;
    unsigned char cstate_control;
    unsigned short worst_c2_latency;
    unsigned short worst_c3_latency;
    unsigned short flush_size;
    unsigned short flush_stride;
    unsigned char duty_offset;
    unsigned char duty_width;
    unsigned char day_alarm;
    unsigned char month_alarm;
    unsigned char century;
    unsigned short boot_arch_flags;
    unsigned char reserved2;
    unsigned int flags;
} __attribute__((packed)) acpi_fadt_t;

void power_off(void);

#endif
