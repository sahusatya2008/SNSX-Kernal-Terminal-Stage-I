#ifndef SNSX_ATA_H
#define SNSX_ATA_H

#include "types.h"

bool ata_init(void);
bool ata_is_ready(void);
bool ata_read(uint32_t lba, uint32_t sector_count, void *buffer);

#endif
