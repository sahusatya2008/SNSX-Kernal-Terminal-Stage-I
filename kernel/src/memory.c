#include "memory.h"

#include "strings.h"

#define SNSX_MAX_MAPPED_BYTES (64u * 1024u * 1024u)
#define SNSX_MAX_PAGES (SNSX_MAX_MAPPED_BYTES / SNSX_PAGE_SIZE)
#define SNSX_HEAP_PAGES 256u

typedef struct HeapBlock {
    size_t size;
    bool free;
    struct HeapBlock *next;
    struct HeapBlock *prev;
} HeapBlock;

static uint8_t page_bitmap[SNSX_MAX_PAGES / 8u];
static size_t total_page_count = 0;
static HeapBlock *heap_head = (HeapBlock *)0;
static size_t heap_total_bytes = 0;

static uintptr_t align_down_uintptr(uintptr_t value, uintptr_t alignment) {
    return value & ~(alignment - 1u);
}

static uintptr_t align_up_uintptr(uintptr_t value, uintptr_t alignment) {
    return (value + alignment - 1u) & ~(alignment - 1u);
}

static bool bitmap_test(size_t index) {
    return (page_bitmap[index / 8u] & (uint8_t)(1u << (index % 8u))) != 0u;
}

static void bitmap_set(size_t index, bool used) {
    const uint8_t mask = (uint8_t)(1u << (index % 8u));
    if (used) {
        page_bitmap[index / 8u] |= mask;
    } else {
        page_bitmap[index / 8u] &= (uint8_t)~mask;
    }
}

static void mark_range(uintptr_t start, uintptr_t end, bool used) {
    const size_t first_page = (size_t)(align_down_uintptr(start, SNSX_PAGE_SIZE) / SNSX_PAGE_SIZE);
    size_t last_page = (size_t)(align_up_uintptr(end, SNSX_PAGE_SIZE) / SNSX_PAGE_SIZE);

    if (last_page > total_page_count) {
        last_page = total_page_count;
    }

    for (size_t page = first_page; page < last_page; ++page) {
        bitmap_set(page, used);
    }
}

static size_t count_used_pages(void) {
    size_t used_pages = 0;
    for (size_t page = 0; page < total_page_count; ++page) {
        if (bitmap_test(page)) {
            ++used_pages;
        }
    }
    return used_pages;
}

static void heap_init(void) {
    void *heap_region = page_alloc(SNSX_HEAP_PAGES);
    if (heap_region == (void *)0) {
        heap_head = (HeapBlock *)0;
        heap_total_bytes = 0;
        return;
    }

    heap_total_bytes = SNSX_HEAP_PAGES * SNSX_PAGE_SIZE;
    heap_head = (HeapBlock *)heap_region;
    heap_head->size = heap_total_bytes - sizeof(HeapBlock);
    heap_head->free = true;
    heap_head->next = (HeapBlock *)0;
    heap_head->prev = (HeapBlock *)0;
}

void memory_init(const BootInfo *boot_info, uintptr_t kernel_end) {
    uint32_t mapped_memory_mb = 64u;

    memory_set(page_bitmap, 0, sizeof(page_bitmap));

    if (boot_info != (const BootInfo *)0 && boot_info->mapped_memory_mb != 0u) {
        mapped_memory_mb = boot_info->mapped_memory_mb;
    }

    if (mapped_memory_mb > 64u) {
        mapped_memory_mb = 64u;
    }

    total_page_count = (size_t)((mapped_memory_mb * 1024u * 1024u) / SNSX_PAGE_SIZE);

    const uintptr_t reserved_top = align_up_uintptr(kernel_end > 0x00100000u ? kernel_end : 0x00100000u, SNSX_PAGE_SIZE);
    mark_range(0u, reserved_top, true);
    mark_range(SNSX_APP_WINDOW_START, SNSX_APP_WINDOW_START + SNSX_APP_WINDOW_SIZE, true);

    heap_init();
}

void *page_alloc(size_t page_count) {
    if (page_count == 0u) {
        return (void *)0;
    }

    size_t run_start = 0;
    size_t run_length = 0;

    for (size_t page = 0; page < total_page_count; ++page) {
        if (!bitmap_test(page)) {
            if (run_length == 0u) {
                run_start = page;
            }
            ++run_length;

            if (run_length == page_count) {
                for (size_t mark = run_start; mark < run_start + page_count; ++mark) {
                    bitmap_set(mark, true);
                }
                return (void *)(uintptr_t)(run_start * SNSX_PAGE_SIZE);
            }
        } else {
            run_length = 0u;
        }
    }

    return (void *)0;
}

void page_free(void *address, size_t page_count) {
    if (address == (void *)0 || page_count == 0u) {
        return;
    }

    size_t page = (size_t)((uintptr_t)address / SNSX_PAGE_SIZE);
    for (size_t index = 0; index < page_count && page + index < total_page_count; ++index) {
        bitmap_set(page + index, false);
    }
}

void *kmalloc(size_t size) {
    if (size == 0u || heap_head == (HeapBlock *)0) {
        return (void *)0;
    }

    size = align_up_uintptr(size, 16u);

    for (HeapBlock *block = heap_head; block != (HeapBlock *)0; block = block->next) {
        if (!block->free || block->size < size) {
            continue;
        }

        if (block->size >= size + sizeof(HeapBlock) + 16u) {
            HeapBlock *split = (HeapBlock *)((uintptr_t)block + sizeof(HeapBlock) + size);
            split->size = block->size - size - sizeof(HeapBlock);
            split->free = true;
            split->next = block->next;
            split->prev = block;

            if (split->next != (HeapBlock *)0) {
                split->next->prev = split;
            }

            block->next = split;
            block->size = size;
        }

        block->free = false;
        return (void *)((uintptr_t)block + sizeof(HeapBlock));
    }

    return (void *)0;
}

void kfree(void *ptr) {
    if (ptr == (void *)0) {
        return;
    }

    HeapBlock *block = (HeapBlock *)((uintptr_t)ptr - sizeof(HeapBlock));
    block->free = true;

    if (block->next != (HeapBlock *)0 && block->next->free) {
        block->size += sizeof(HeapBlock) + block->next->size;
        block->next = block->next->next;
        if (block->next != (HeapBlock *)0) {
            block->next->prev = block;
        }
    }

    if (block->prev != (HeapBlock *)0 && block->prev->free) {
        block->prev->size += sizeof(HeapBlock) + block->size;
        block->prev->next = block->next;
        if (block->next != (HeapBlock *)0) {
            block->next->prev = block->prev;
        }
    }
}

uint64_t memory_total_bytes(void) {
    return (uint64_t)total_page_count * SNSX_PAGE_SIZE;
}

uint64_t memory_used_bytes(void) {
    return (uint64_t)count_used_pages() * SNSX_PAGE_SIZE;
}

uint64_t memory_free_bytes(void) {
    return memory_total_bytes() - memory_used_bytes();
}

uint64_t memory_heap_bytes(void) {
    return heap_total_bytes;
}

uint64_t memory_heap_free_bytes(void) {
    uint64_t free_bytes = 0;

    for (HeapBlock *block = heap_head; block != (HeapBlock *)0; block = block->next) {
        if (block->free) {
            free_bytes += block->size;
        }
    }

    return free_bytes;
}
