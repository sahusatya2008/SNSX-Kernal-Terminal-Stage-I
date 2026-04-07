#include "ata.h"

#include "ports.h"

#define ATA_PRIMARY_IO 0x1F0
#define ATA_PRIMARY_CTRL 0x3F6

#define ATA_REG_DATA       0
#define ATA_REG_SECTOR_CNT 2
#define ATA_REG_LBA_LOW    3
#define ATA_REG_LBA_MID    4
#define ATA_REG_LBA_HIGH   5
#define ATA_REG_DRIVE      6
#define ATA_REG_COMMAND    7
#define ATA_REG_STATUS     7

#define ATA_STATUS_ERR 0x01
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_DF  0x20
#define ATA_STATUS_DRDY 0x40
#define ATA_STATUS_BSY 0x80

#define ATA_CMD_READ_SECTORS 0x20

static bool driver_ready = false;

static uint8_t ata_status(void) {
    return inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
}

static bool ata_wait_not_busy(void) {
    for (uint32_t spin = 0; spin < 100000u; ++spin) {
        if ((ata_status() & ATA_STATUS_BSY) == 0u) {
            return true;
        }
    }
    return false;
}

static bool ata_wait_for_data(void) {
    for (uint32_t spin = 0; spin < 100000u; ++spin) {
        const uint8_t status = ata_status();
        if ((status & ATA_STATUS_ERR) != 0u || (status & ATA_STATUS_DF) != 0u) {
            return false;
        }
        if ((status & ATA_STATUS_BSY) == 0u && (status & ATA_STATUS_DRQ) != 0u) {
            return true;
        }
    }
    return false;
}

bool ata_init(void) {
    outb(ATA_PRIMARY_CTRL, 0x00);
    driver_ready = ata_wait_not_busy();
    return driver_ready;
}

bool ata_is_ready(void) {
    return driver_ready;
}

bool ata_read(uint32_t lba, uint32_t sector_count, void *buffer) {
    if (!driver_ready || buffer == (void *)0 || sector_count == 0u) {
        return false;
    }

    uint16_t *data = (uint16_t *)buffer;
    uint32_t remaining = sector_count;
    uint32_t current_lba = lba;

    while (remaining > 0u) {
        uint8_t chunk = remaining > 255u ? 255u : (uint8_t)remaining;

        if (!ata_wait_not_busy()) {
            return false;
        }

        outb(ATA_PRIMARY_IO + ATA_REG_DRIVE, (uint8_t)(0xE0u | ((current_lba >> 24) & 0x0Fu)));
        outb(ATA_PRIMARY_IO + ATA_REG_SECTOR_CNT, chunk);
        outb(ATA_PRIMARY_IO + ATA_REG_LBA_LOW, (uint8_t)(current_lba & 0xFFu));
        outb(ATA_PRIMARY_IO + ATA_REG_LBA_MID, (uint8_t)((current_lba >> 8) & 0xFFu));
        outb(ATA_PRIMARY_IO + ATA_REG_LBA_HIGH, (uint8_t)((current_lba >> 16) & 0xFFu));
        outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);

        for (uint8_t sector = 0; sector < chunk; ++sector) {
            if (!ata_wait_for_data()) {
                return false;
            }

            for (uint16_t word = 0; word < 256u; ++word) {
                data[word] = __builtin_bswap16(__builtin_bswap16((uint16_t)__extension__({
                    uint16_t value;
                    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"((uint16_t)(ATA_PRIMARY_IO + ATA_REG_DATA)));
                    value;
                })));
            }

            data += 256;
        }

        current_lba += chunk;
        remaining -= chunk;
    }

    return true;
}
