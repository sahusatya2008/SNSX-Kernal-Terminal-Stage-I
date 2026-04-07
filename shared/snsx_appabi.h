#ifndef SNSX_APPABI_SHARED_H
#define SNSX_APPABI_SHARED_H

#include <stdint.h>

typedef struct SnsxAppApi {
    void (*write)(const char *text);
    void (*writeln)(const char *text);
    void (*write_u64)(uint64_t value);
    void (*write_hex32)(uint32_t value);
    uint64_t (*uptime_ticks)(void);
    uint64_t (*memory_total_bytes)(void);
    uint64_t (*memory_used_bytes)(void);
} SnsxAppApi;

typedef int (*SnsxAppEntry)(const SnsxAppApi *api);

#endif
