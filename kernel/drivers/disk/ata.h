#ifndef ATA_H
#define ATA_H

#include "../../helpers/ports/ports.h"

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

extern ATA_Driver *fs_drv;

int srst_ata_st(unsigned short dcr_port, unsigned short tf_port);

int ata_wait_ready(unsigned short port, unsigned int timeout_ms);
int ata_check_present(unsigned short port);
int ata_wait_drq(unsigned short port, unsigned int timeout_ms);
int ata_check_error(ATA_Driver *drv);

int pio28_read(unsigned int *lba, unsigned char *buf, unsigned char sectors);

int fs_disk_read(unsigned int lba, unsigned int sectors, void *buffer);
int fs_disk_write(unsigned int lba, unsigned int sectors, void *buffer);

int ata_identify(ATA_Driver *drv);
int ata_init(ATA_Driver *drv, unsigned short tf_port, unsigned short dcr_port, unsigned char sbits);

#endif
