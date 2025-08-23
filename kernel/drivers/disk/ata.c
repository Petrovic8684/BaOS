#include "ata.h"

ATA_Driver *fs_drv;

int srst_ata_st(unsigned short dcr_port, unsigned short tf_port)
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

int ata_wait_ready(unsigned short port, unsigned int timeout_ms)
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

int ata_check_present(unsigned short port)
{
    unsigned char status = inb(port + 7);

    if (status == 0xFF)
        return -1;

    return 0;
}

int ata_wait_drq(unsigned short port, unsigned int timeout_ms)
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

int ata_check_error(ATA_Driver *drv)
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

int ata_identify(ATA_Driver *drv)
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
    drv->tf = tf_port;
    drv->dcr = dcr_port;
    drv->sbits = sbits;
    drv->stLBA = 0;
    drv->prtlen = 8;
    drv->exists = 0;
    drv->error = 0;

    if (srst_ata_st(dcr_port, tf_port) != 0)
        return -1;

    if (ata_identify(drv) != 0)
        return -1;

    return 0;
}

int pio28_read(unsigned int *lba, unsigned char *buf, unsigned char sectors)
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

    outb(port + 2, sectors);
    outb(port + 3, abs_lba & 0xFF);
    outb(port + 4, (abs_lba >> 8) & 0xFF);
    outb(port + 5, (abs_lba >> 16) & 0xFF);
    outb(port + 6, ((abs_lba >> 24) & 0x0F) | fs_drv->sbits | 0xE0);
    outb(port + 7, 0x20);

    for (unsigned int i = 0; i < eff; i++)
    {
        int r = ata_wait_drq(port, 5000);

        if (r != 0)
            return -4;

        if (ata_check_error(fs_drv) != 0)
            return -5;

        insw(port, (unsigned short *)(buf + i * SECTOR_SIZE), 256);
    }

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
    unsigned short port = fs_drv->tf;

    while (sectors)
    {
        unsigned char count = sectors > 256 ? 256 : (unsigned char)sectors;
        unsigned int abs_lba = lba + fs_drv->stLBA;

        outb(port + 2, count);
        outb(port + 3, abs_lba & 0xFF);
        outb(port + 4, (abs_lba >> 8) & 0xFF);
        outb(port + 5, (abs_lba >> 16) & 0xFF);
        outb(port + 6, ((abs_lba >> 24) & 0x0F) | fs_drv->sbits | 0xE0);
        outb(port + 7, 0x30);

        for (int i = 0; i < count; i++)
        {
            int r = ata_wait_drq(port, 5000);
            if (r != 0)
                return -2;

            if (ata_check_error(fs_drv) != 0)
                return -5;

            outsw(port, (const unsigned short *)(buf + i * SECTOR_SIZE), 256);
        }

        lba += count;
        buf += count * SECTOR_SIZE;
        sectors -= count;
    }

    outb(port + 7, 0xE7);

    if (ata_wait_ready(port, 5000) != 0)
        return -6;

    if (ata_check_error(fs_drv) != 0)
        return -7;

    return 0;
}