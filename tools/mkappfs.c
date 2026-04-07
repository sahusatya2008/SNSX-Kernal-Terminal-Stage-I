#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct AppFsHeader {
    char magic[8];
    uint32_t version;
    uint32_t entry_count;
    uint32_t header_size;
    uint32_t image_size;
} AppFsHeader;

typedef struct AppFsEntry {
    char name[32];
    uint32_t offset;
    uint32_t size;
    uint32_t reserved;
} AppFsEntry;

static uint8_t *read_file(const char *path, size_t *size_out) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        return NULL;
    }

    rewind(file);

    uint8_t *buffer = (uint8_t *)malloc((size_t)size);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, (size_t)size, file) != (size_t)size) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    fclose(file);
    *size_out = (size_t)size;
    return buffer;
}

int main(int argc, char **argv) {
    if (argc < 4 || ((argc - 2) % 2) != 0) {
        fprintf(stderr, "usage: %s OUTPUT NAME FILE [NAME FILE]...\n", argv[0]);
        return 1;
    }

    const int entry_count = (argc - 2) / 2;
    AppFsHeader header = {
        .magic = { 'S', 'A', 'P', 'P', 'F', 'S', '1', '\0' },
        .version = 1u,
        .entry_count = (uint32_t)entry_count,
        .header_size = (uint32_t)(sizeof(AppFsHeader) + (sizeof(AppFsEntry) * (size_t)entry_count)),
        .image_size = 0u
    };

    AppFsEntry *entries = (AppFsEntry *)calloc((size_t)entry_count, sizeof(AppFsEntry));
    uint8_t **payloads = (uint8_t **)calloc((size_t)entry_count, sizeof(uint8_t *));
    size_t *sizes = (size_t *)calloc((size_t)entry_count, sizeof(size_t));

    if (entries == NULL || payloads == NULL || sizes == NULL) {
        fprintf(stderr, "allocation failed\n");
        free(entries);
        free(payloads);
        free(sizes);
        return 1;
    }

    uint32_t running_offset = header.header_size;

    for (int index = 0; index < entry_count; ++index) {
        const char *name = argv[2 + index * 2];
        const char *path = argv[3 + index * 2];

        payloads[index] = read_file(path, &sizes[index]);
        if (payloads[index] == NULL) {
            fprintf(stderr, "failed to read %s\n", path);
            return 1;
        }

        strncpy(entries[index].name, name, sizeof(entries[index].name) - 1u);
        entries[index].offset = running_offset;
        entries[index].size = (uint32_t)sizes[index];
        entries[index].reserved = 0u;
        running_offset += (uint32_t)sizes[index];
    }

    header.image_size = running_offset;

    FILE *output = fopen(argv[1], "wb");
    if (output == NULL) {
        fprintf(stderr, "failed to open %s\n", argv[1]);
        return 1;
    }

    fwrite(&header, sizeof(header), 1, output);
    fwrite(entries, sizeof(AppFsEntry), (size_t)entry_count, output);

    for (int index = 0; index < entry_count; ++index) {
        fwrite(payloads[index], 1, sizes[index], output);
        free(payloads[index]);
    }

    fclose(output);
    free(entries);
    free(payloads);
    free(sizes);
    return 0;
}
