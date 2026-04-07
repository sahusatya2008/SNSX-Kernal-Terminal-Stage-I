#include "runtime.h"

#include "appfs.h"
#include "memory.h"
#include "pit.h"
#include "serial.h"
#include "strings.h"
#include "terminal.h"
#include "snsx_appabi.h"

#define ELF_MAGIC 0x464C457Fu
#define ELF_CLASS_64 2u
#define ELF_DATA_LITTLE 1u
#define ELF_TYPE_EXEC 2u
#define ELF_MACHINE_X86_64 62u
#define ELF_PT_LOAD 1u

typedef struct Elf64Header {
    uint32_t magic;
    uint8_t class_id;
    uint8_t data_encoding;
    uint8_t version;
    uint8_t abi;
    uint8_t abi_version;
    uint8_t pad[7];
    uint16_t type;
    uint16_t machine;
    uint32_t elf_version;
    uint64_t entry;
    uint64_t program_header_offset;
    uint64_t section_header_offset;
    uint32_t flags;
    uint16_t header_size;
    uint16_t program_header_entry_size;
    uint16_t program_header_count;
    uint16_t section_header_entry_size;
    uint16_t section_header_count;
    uint16_t section_name_index;
} __attribute__((packed)) Elf64Header;

typedef struct Elf64ProgramHeader {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t virtual_address;
    uint64_t physical_address;
    uint64_t file_size;
    uint64_t memory_size;
    uint64_t alignment;
} __attribute__((packed)) Elf64ProgramHeader;

static bool runtime_ready = false;
static uint32_t last_exit_code = 0;

static void api_write(const char *text) {
    terminal_write(text);
    serial_write(text);
}

static void api_writeln(const char *text) {
    terminal_writeln(text);
    serial_writeln(text);
}

static void api_write_u64(uint64_t value) {
    terminal_write_uint64(value);
    serial_write_uint64(value);
}

static void api_write_hex32(uint32_t value) {
    terminal_write_hex32(value);
    serial_write_hex32(value);
}

static uint64_t api_uptime_ticks(void) {
    return pit_ticks();
}

static uint64_t api_memory_total_bytes(void) {
    return memory_total_bytes();
}

static uint64_t api_memory_used_bytes(void) {
    return memory_used_bytes();
}

static const SnsxAppApi runtime_api = {
    .write = api_write,
    .writeln = api_writeln,
    .write_u64 = api_write_u64,
    .write_hex32 = api_write_hex32,
    .uptime_ticks = api_uptime_ticks,
    .memory_total_bytes = api_memory_total_bytes,
    .memory_used_bytes = api_memory_used_bytes
};

static bool runtime_validate_elf(const uint8_t *image, size_t size, const Elf64Header **header_out) {
    if (size < sizeof(Elf64Header)) {
        return false;
    }

    const Elf64Header *header = (const Elf64Header *)image;
    if (header->magic != ELF_MAGIC ||
        header->class_id != ELF_CLASS_64 ||
        header->data_encoding != ELF_DATA_LITTLE ||
        header->type != ELF_TYPE_EXEC ||
        header->machine != ELF_MACHINE_X86_64) {
        return false;
    }

    const uint64_t ph_end =
        header->program_header_offset +
        ((uint64_t)header->program_header_entry_size * header->program_header_count);

    if (ph_end > size) {
        return false;
    }

    *header_out = header;
    return true;
}

static bool runtime_load_segments(const uint8_t *image, size_t size, const Elf64Header *header) {
    const Elf64ProgramHeader *program_headers =
        (const Elf64ProgramHeader *)(image + header->program_header_offset);

    memory_set((void *)(uintptr_t)SNSX_APP_WINDOW_START, 0, SNSX_APP_WINDOW_SIZE);

    for (uint16_t index = 0; index < header->program_header_count; ++index) {
        const Elf64ProgramHeader *program = &program_headers[index];

        if (program->type != ELF_PT_LOAD || program->memory_size == 0u) {
            continue;
        }

        if (program->offset + program->file_size > size) {
            return false;
        }

        if (program->virtual_address < SNSX_APP_WINDOW_START ||
            program->virtual_address + program->memory_size > SNSX_APP_WINDOW_START + SNSX_APP_WINDOW_SIZE) {
            return false;
        }

        memory_copy((void *)(uintptr_t)program->virtual_address, image + program->offset, (size_t)program->file_size);
        if (program->memory_size > program->file_size) {
            memory_set(
                (void *)(uintptr_t)(program->virtual_address + program->file_size),
                0,
                (size_t)(program->memory_size - program->file_size));
        }
    }

    return true;
}

bool runtime_init(const BootInfo *boot_info) {
    runtime_ready = appfs_init(boot_info);
    return runtime_ready;
}

bool runtime_is_ready(void) {
    return runtime_ready;
}

uint32_t runtime_app_count(void) {
    return runtime_is_ready() ? appfs_entry_count() : 0u;
}

const char *runtime_app_name(uint32_t index) {
    const AppFsEntry *entry = appfs_entry_at(index);
    return entry != (const AppFsEntry *)0 ? entry->name : "";
}

uint32_t runtime_app_size(uint32_t index) {
    const AppFsEntry *entry = appfs_entry_at(index);
    return entry != (const AppFsEntry *)0 ? entry->size : 0u;
}

bool runtime_launch(const char *name) {
    if (!runtime_is_ready()) {
        return false;
    }

    const AppFsEntry *entry = appfs_find(name);
    if (entry == (const AppFsEntry *)0) {
        return false;
    }

    const uint8_t *image = appfs_data_at(entry);
    if (image == (const uint8_t *)0) {
        return false;
    }

    const Elf64Header *header = (const Elf64Header *)0;
    if (!runtime_validate_elf(image, entry->size, &header)) {
        serial_writeln("[SNSX] ELF validation failed");
        return false;
    }

    if (!runtime_load_segments(image, entry->size, header)) {
        serial_writeln("[SNSX] ELF segment load failed");
        return false;
    }

    serial_write("[SNSX] launching app: ");
    serial_writeln(name);

    terminal_write("Launching       : ");
    terminal_writeln(name);

    const SnsxAppEntry app_entry = (SnsxAppEntry)(uintptr_t)header->entry;
    last_exit_code = (uint32_t)app_entry(&runtime_api);

    serial_write("[SNSX] app exit code: ");
    serial_write_uint64(last_exit_code);
    serial_write("\n");

    return true;
}

void runtime_autorun(void) {
    if (!runtime_is_ready()) {
        serial_writeln("[SNSX] autorun skipped: runtime not ready");
        return;
    }

    const uint32_t app_count = runtime_app_count();
    serial_write("[SNSX] runtime app count: ");
    serial_write_uint64(app_count);
    serial_write("\n");

    if (app_count == 0u) {
        serial_writeln("[SNSX] autorun skipped: no packaged apps");
        return;
    }

    const char *name = runtime_app_name(0);
    serial_write("[SNSX] autorun target: ");
    serial_writeln(name);

    terminal_write("Runtime self-test: launching ");
    terminal_write(name);
    terminal_writeln(".");
    runtime_launch(name);
    terminal_writeln("");
}

uint32_t runtime_last_exit_code(void) {
    return last_exit_code;
}

uint32_t runtime_storage_lba(void) {
    return appfs_lba();
}

uint32_t runtime_storage_sectors(void) {
    return appfs_sectors();
}
