#ifndef SNSX_RUNTIME_H
#define SNSX_RUNTIME_H

#include "boot_info.h"
#include "types.h"

bool runtime_init(const BootInfo *boot_info);
bool runtime_is_ready(void);
uint32_t runtime_app_count(void);
const char *runtime_app_name(uint32_t index);
uint32_t runtime_app_size(uint32_t index);
bool runtime_launch(const char *name);
void runtime_autorun(void);
uint32_t runtime_last_exit_code(void);
uint32_t runtime_storage_lba(void);
uint32_t runtime_storage_sectors(void);

#endif
