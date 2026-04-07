#ifndef SNSX_APPFS_H
#define SNSX_APPFS_H

#include "boot_info.h"
#include "types.h"

typedef struct AppFsHeader {
    char magic[8];
    uint32_t version;
    uint32_t entry_count;
    uint32_t header_size;
    uint32_t image_size;
} __attribute__((packed)) AppFsHeader;

typedef struct AppFsEntry {
    char name[32];
    uint32_t offset;
    uint32_t size;
    uint32_t reserved;
} __attribute__((packed)) AppFsEntry;

bool appfs_init(const BootInfo *boot_info);
bool appfs_is_ready(void);
uint32_t appfs_entry_count(void);
const AppFsEntry *appfs_entry_at(uint32_t index);
const AppFsEntry *appfs_find(const char *name);
const uint8_t *appfs_data_at(const AppFsEntry *entry);
uint32_t appfs_lba(void);
uint32_t appfs_sectors(void);
uint32_t appfs_size(void);

#endif
