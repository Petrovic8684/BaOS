#ifndef ATA_H
#define ATA_H

#define SECTOR_SIZE 512

typedef struct ATA_Driver
{
    unsigned short tf;
    unsigned short dcr;
    unsigned int stLBA;
    unsigned int prtlen;
    unsigned char sbits;
    unsigned char exists;
    unsigned int sectors;
    unsigned char error;
} ATA_Driver;

void ata_set_fs_drv(ATA_Driver *drv);
ATA_Driver *ata_get_fs_drv(void);

int ata_init(ATA_Driver *drv, unsigned short tf_port, unsigned short dcr_port, unsigned char sbits);

int fs_disk_read(unsigned int lba, unsigned int sectors, void *buffer);
int fs_disk_write(unsigned int lba, unsigned int sectors, void *buffer);

#endif
