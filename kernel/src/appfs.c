#include "appfs.h"

#include "ata.h"
#include "memory.h"
#include "strings.h"

static uint8_t *appfs_image = (uint8_t *)0;
static AppFsHeader *appfs_header = (AppFsHeader *)0;
static uint32_t boot_lba = 0;
static uint32_t boot_sectors = 0;

static bool appfs_validate(void) {
    static const char expected_magic[8] = { 'S', 'A', 'P', 'P', 'F', 'S', '1', '\0' };

    if (appfs_header == (AppFsHeader *)0) {
        return false;
    }

    if (memory_compare(appfs_header->magic, expected_magic, sizeof(expected_magic)) != 0) {
        return false;
    }

    if (appfs_header->version != 1u) {
        return false;
    }

    if (appfs_header->header_size < sizeof(AppFsHeader) + (appfs_header->entry_count * sizeof(AppFsEntry))) {
        return false;
    }

    if (appfs_header->image_size > boot_sectors * 512u) {
        return false;
    }

    return true;
}

bool appfs_init(const BootInfo *boot_info) {
    if (boot_info == (const BootInfo *)0 || boot_info->appfs_sectors == 0u) {
        return false;
    }

    boot_lba = boot_info->appfs_lba;
    boot_sectors = boot_info->appfs_sectors;

    if (!ata_init()) {
        return false;
    }

    const size_t page_count = (size_t)((boot_sectors * 512u + SNSX_PAGE_SIZE - 1u) / SNSX_PAGE_SIZE);
    appfs_image = (uint8_t *)page_alloc(page_count);
    if (appfs_image == (uint8_t *)0) {
        return false;
    }

    if (!ata_read(boot_lba, boot_sectors, appfs_image)) {
        return false;
    }

    appfs_header = (AppFsHeader *)appfs_image;
    return appfs_validate();
}

bool appfs_is_ready(void) {
    return appfs_validate();
}

uint32_t appfs_entry_count(void) {
    return appfs_is_ready() ? appfs_header->entry_count : 0u;
}

const AppFsEntry *appfs_entry_at(uint32_t index) {
    if (!appfs_is_ready() || index >= appfs_header->entry_count) {
        return (const AppFsEntry *)0;
    }

    const AppFsEntry *entries = (const AppFsEntry *)(appfs_image + sizeof(AppFsHeader));
    return &entries[index];
}

const AppFsEntry *appfs_find(const char *name) {
    for (uint32_t index = 0; index < appfs_entry_count(); ++index) {
        const AppFsEntry *entry = appfs_entry_at(index);
        if (entry != (const AppFsEntry *)0 && string_compare(entry->name, name) == 0) {
            return entry;
        }
    }
    return (const AppFsEntry *)0;
}

const uint8_t *appfs_data_at(const AppFsEntry *entry) {
    if (!appfs_is_ready() || entry == (const AppFsEntry *)0) {
        return (const uint8_t *)0;
    }

    if (entry->offset + entry->size > appfs_header->image_size) {
        return (const uint8_t *)0;
    }

    return appfs_image + entry->offset;
}

uint32_t appfs_lba(void) {
    return boot_lba;
}

uint32_t appfs_sectors(void) {
    return boot_sectors;
}

uint32_t appfs_size(void) {
    return appfs_is_ready() ? appfs_header->image_size : 0u;
}
