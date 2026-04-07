#ifndef SNSX_BOOT_INFO_H
#define SNSX_BOOT_INFO_H

#include "types.h"

#define SNSX_BOOTINFO_MAGIC 0x534E5358u
#define SNSX_BOOT_FLAG_BIOS 0x00000001u

typedef struct BootInfo {
    uint32_t magic;
    uint32_t stage2_sectors;
    uint32_t kernel_sectors;
    uint32_t flags;
    uint8_t boot_drive;
    uint8_t reserved[3];
    uint32_t appfs_lba;
    uint32_t appfs_sectors;
    uint32_t mapped_memory_mb;
} BootInfo;

#endif
