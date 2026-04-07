#ifndef SNSX_MEMORY_H
#define SNSX_MEMORY_H

#include "boot_info.h"
#include "types.h"

#define SNSX_PAGE_SIZE 4096u
#define SNSX_APP_WINDOW_START 0x00400000u
#define SNSX_APP_WINDOW_SIZE  0x00400000u

void memory_init(const BootInfo *boot_info, uintptr_t kernel_end);
void *page_alloc(size_t page_count);
void page_free(void *address, size_t page_count);
void *kmalloc(size_t size);
void kfree(void *ptr);
uint64_t memory_total_bytes(void);
uint64_t memory_used_bytes(void);
uint64_t memory_free_bytes(void);
uint64_t memory_heap_bytes(void);
uint64_t memory_heap_free_bytes(void);

#endif
