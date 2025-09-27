#include "ata.h"
#include "../display/display.h"
#include "../pit/pit.h"
#include "../../helpers/ports/ports.h"

static ATA_Driver *fs_drv = 0;

static int srst_ata_st(unsigned short dcr_port, unsigned short tf_port)
{
    outb(dcr_port, 0x04);
    inb(dcr_port);
    inb(dcr_port);
    inb(dcr_port);
    inb(dcr_port);

    outb(dcr_port, 0x00);
    inb(dcr_port);
    inb(dcr_port);
    inb(dcr_port);
    inb(dcr_port);

    unsigned int t = 0;
    const unsigned int timeout = 5000;

    while (t < timeout)
    {
        unsigned char status = inb(tf_port + 7);

        if ((status & 0xC0) == 0x40)
            return 0;

        for (volatile int i = 0; i < 1000; i++)
            ;
        t++;
    }

    return -1;
}

static int ata_wait_ready(unsigned short port, unsigned int timeout_ms)
{
    unsigned int t = 0;

    while (t < timeout_ms)
    {
        unsigned char status = inb(port + 7);

        if (!(status & 0x80))
            return 0;

        for (volatile int i = 0; i < 1000; i++)
            ;
        t++;
    }

    return -1;
}

static int ata_check_present(unsigned short port)
{
    unsigned char status = inb(port + 7);

    if (status == 0xFF)
        return -1;

    return 0;
}

static int ata_wait_drq(unsigned short port, unsigned int timeout_ms)
{
    unsigned int t = 0;

    while (t < timeout_ms)
    {
        unsigned char status = inb(port + 7);

        if (status & 0x01)
            return -1;

        if (!(status & 0x80) && (status & 0x08))
            return 0;

        for (volatile int i = 0; i < 1000; i++)
            ;
        t++;
    }

    return -2;
}

static int ata_check_error(ATA_Driver *drv)
{
    unsigned char status = inb(drv->tf + 7);

    if (status & 0x01)
    {
        drv->error = inb(drv->tf + 1);
        return -1;
    }

    drv->error = 0;
    return 0;
}

static int ata_identify(ATA_Driver *drv)
{
    unsigned short port = drv->tf;

    outb(port + 6, drv->sbits | 0xA0);
    for (volatile int i = 0; i < 1000; i++)
        ;

    outb(port + 7, 0xEC);

    int r = ata_wait_drq(port, 5000);
    if (r != 0)
        return -1;

    unsigned short data[256];
    insw(port, data, 256);

    drv->sectors = ((unsigned int)data[60]) | (((unsigned int)data[61]) << 16);
    drv->exists = 1;

    return 0;
}

int ata_init(ATA_Driver *drv, unsigned short tf_port, unsigned short dcr_port, unsigned char sbits)
{
    write("Initializing ATA driver...\033[0m\n");

    if (!drv)
    {
        write("\033[31mError: ATA driver initialization failed (drv pointer is NULL). Halting...\033[0m\n\n");
        __asm__ volatile(
            "cli\n\t"
            "hlt\n\t");
        while (1)
            ;
    }

    drv->tf = tf_port;
    drv->dcr = dcr_port;
    drv->sbits = sbits;
    drv->stLBA = 0;
    drv->prtlen = 8;
    drv->exists = 0;
    drv->error = 0;

    if (srst_ata_st(dcr_port, tf_port) != 0)
    {
        write("\033[31mError: ATA driver initialization failed (soft reset failed). Halting...\033[0m\n\n");
        __asm__ volatile(
            "cli\n\t"
            "hlt\n\t");
        while (1)
            ;
    }

    if (ata_identify(drv) != 0)
    {
        write("\033[31mError: ATA driver initialization failed (no device present or not responding). Halting...\033[0m\n\n");
        __asm__ volatile(
            "cli\n\t"
            "hlt\n\t");
        while (1)
            ;
    }

    write("\033[32mATA driver initialized.\033[0m\n");
    return 0;
}

typedef enum
{
    ATA_DIR_READ = 0,
    ATA_DIR_WRITE = 1
} ata_dir_t;

typedef struct
{
    volatile int in_progress;
    volatile int completed;
    volatile int error;
    unsigned char *buf;
    unsigned int sectors_left;
    unsigned int total_sectors;
    unsigned int lba;
    ata_dir_t dir;
} ata_xfer_t;

static volatile ata_xfer_t cur_xfer;

static inline void pic_send_eoi(int irq)
{
    if (irq >= 8)
        outb(0xA0, 0x20);

    outb(0x20, 0x20);
}

void ata_irq_handler(int irq)
{
    (void)irq;

    if (!fs_drv || !fs_drv->exists)
    {
        pic_send_eoi(irq);
        return;
    }

    unsigned short port = fs_drv->tf;
    unsigned char status = inb(port + 7);

    if (status & 0x01)
    {
        fs_drv->error = inb(port + 1);
        cur_xfer.error = 1;
        cur_xfer.in_progress = 0;
        cur_xfer.completed = 1;
        pic_send_eoi(irq);
        return;
    }

    if (status & 0x08)
    {
        if (cur_xfer.dir == ATA_DIR_READ)
        {
            insw(port, (unsigned short *)cur_xfer.buf, 256);
            cur_xfer.buf += SECTOR_SIZE;
            cur_xfer.sectors_left--;
        }
        else
        {
            outsw(port, (const unsigned short *)cur_xfer.buf, 256);
            cur_xfer.buf += SECTOR_SIZE;
            cur_xfer.sectors_left--;
        }

        if (cur_xfer.sectors_left == 0)
        {
            cur_xfer.in_progress = 0;
            cur_xfer.completed = 1;
        }
    }

    pic_send_eoi(irq);
}

static int wait_for_irq_completion(unsigned int timeout_ms)
{
    unsigned int flags;
    __asm__ volatile(
        "pushf\n\t"
        "pop %0"
        : "=r"(flags)
        :
        : "memory");

    int was_enabled = (flags & (1 << 9)) != 0;
    if (!was_enabled)
        __asm__ volatile("sti");

    unsigned long long start = pit_get_ms();
    unsigned long long end_ms = start + (unsigned long long)timeout_ms;

    while (!cur_xfer.completed && pit_get_ms() < end_ms)
        __asm__ volatile("hlt");

    if (!was_enabled)
        __asm__ volatile("cli");

    if (!cur_xfer.completed)
        return -1;
    if (cur_xfer.error)
        return -2;

    return 0;
}

static int pio28_read(unsigned int *lba, unsigned char *buf, unsigned char sectors)
{
    if (!fs_drv || !fs_drv->exists)
        return -1;

    unsigned int abs_lba = *lba + fs_drv->stLBA;
    unsigned short port = fs_drv->tf;

    if (ata_check_present(port) != 0)
        return -2;

    if (ata_wait_ready(port, 500) != 0)
        return -3;

    unsigned int eff = (sectors == 0) ? 256u : (unsigned int)sectors;

    cur_xfer.in_progress = 1;
    cur_xfer.completed = 0;
    cur_xfer.error = 0;
    cur_xfer.buf = buf;
    cur_xfer.sectors_left = eff;
    cur_xfer.total_sectors = eff;
    cur_xfer.lba = abs_lba;
    cur_xfer.dir = ATA_DIR_READ;

    outb(port + 2, (unsigned char)(eff == 256 ? 0 : eff));
    outb(port + 3, abs_lba & 0xFF);
    outb(port + 4, (abs_lba >> 8) & 0xFF);
    outb(port + 5, (abs_lba >> 16) & 0xFF);
    outb(port + 6, ((abs_lba >> 24) & 0x0F) | fs_drv->sbits | 0xE0);
    outb(port + 7, 0x20);

    int r = wait_for_irq_completion(5000);
    if (r != 0)
    {
        unsigned char s = inb(port + 7);
        write("ATA timeout or error, status=0x");
        write_hex(s);
        write("\n");

        cur_xfer.in_progress = 0;
        return -4;
    }

    *lba += eff;
    return 0;
}

static int pio28_write(unsigned int *lba, unsigned char *buf, unsigned char sectors)
{
    if (!fs_drv || !fs_drv->exists)
        return -1;

    unsigned int abs_lba = *lba + fs_drv->stLBA;
    unsigned short port = fs_drv->tf;

    if (ata_check_present(port) != 0)
        return -2;

    if (ata_wait_ready(port, 500) != 0)
        return -3;

    unsigned int eff = (sectors == 0) ? 256u : (unsigned int)sectors;

    cur_xfer.in_progress = 1;
    cur_xfer.completed = 0;
    cur_xfer.error = 0;
    cur_xfer.buf = buf;
    cur_xfer.sectors_left = eff;
    cur_xfer.total_sectors = eff;
    cur_xfer.lba = abs_lba;
    cur_xfer.dir = ATA_DIR_WRITE;

    outb(port + 2, (unsigned char)(eff == 256 ? 0 : eff));
    outb(port + 3, abs_lba & 0xFF);
    outb(port + 4, (abs_lba >> 8) & 0xFF);
    outb(port + 5, (abs_lba >> 16) & 0xFF);
    outb(port + 6, ((abs_lba >> 24) & 0x0F) | fs_drv->sbits | 0xE0);
    outb(port + 7, 0x30);

    if (ata_wait_drq(port, 5000) != 0)
        return -4;

    outsw(port, (const unsigned short *)cur_xfer.buf, 256);
    cur_xfer.buf += SECTOR_SIZE;
    cur_xfer.sectors_left--;

    if (cur_xfer.sectors_left == 0)
    {
        cur_xfer.in_progress = 0;
        cur_xfer.completed = 1;
    }
    else
    {
        int r = wait_for_irq_completion(5000);
        if (r != 0)
        {
            unsigned char s = inb(port + 7);
            write("ATA timeout or error, status=0x");
            write_hex(s);
            write("\n");

            cur_xfer.in_progress = 0;
            return -5;
        }
    }

    outb(port + 7, 0xE7);
    if (ata_wait_ready(port, 5000) != 0)
        return -6;

    if (ata_check_error(fs_drv) != 0)
        return -7;

    *lba += eff;
    return 0;
}

int fs_disk_read(unsigned int lba, unsigned int sectors, void *buffer)
{
    unsigned char *buf = (unsigned char *)buffer;

    while (sectors)
    {
        unsigned char count = sectors > 256 ? 256 : (unsigned char)sectors;

        int r = pio28_read(&lba, buf, count);
        if (r != 0)
            return r;

        buf += count * SECTOR_SIZE;
        sectors -= count;
    }

    return 0;
}

int fs_disk_write(unsigned int lba, unsigned int sectors, void *buffer)
{
    unsigned char *buf = (unsigned char *)buffer;

    while (sectors)
    {
        unsigned char count = sectors > 256 ? 256 : (unsigned char)sectors;

        int r = pio28_write(&lba, buf, count);
        if (r != 0)
            return r;

        buf += count * SECTOR_SIZE;
        sectors -= count;
    }

    return 0;
}

void ata_set_fs_drv(ATA_Driver *drv)
{
    fs_drv = drv;
}

ATA_Driver *ata_get_fs_drv(void)
{
    return fs_drv;
}