#include "strings.h"

size_t string_length(const char *text) {
    size_t length = 0;
    while (text[length] != '\0') {
        ++length;
    }
    return length;
}

int string_compare(const char *left, const char *right) {
    while (*left != '\0' && *right != '\0') {
        if (*left != *right) {
            return (int)((unsigned char)*left - (unsigned char)*right);
        }
        ++left;
        ++right;
    }
    return (int)((unsigned char)*left - (unsigned char)*right);
}

int string_ncompare(const char *left, const char *right, size_t count) {
    for (size_t index = 0; index < count; ++index) {
        if (left[index] != right[index]) {
            return (int)((unsigned char)left[index] - (unsigned char)right[index]);
        }
        if (left[index] == '\0' || right[index] == '\0') {
            return 0;
        }
    }
    return 0;
}

bool string_starts_with(const char *text, const char *prefix) {
    while (*prefix != '\0') {
        if (*text != *prefix) {
            return false;
        }
        ++text;
        ++prefix;
    }
    return true;
}

void *memory_copy(void *destination, const void *source, size_t count) {
    uint8_t *dest = (uint8_t *)destination;
    const uint8_t *src = (const uint8_t *)source;

    for (size_t index = 0; index < count; ++index) {
        dest[index] = src[index];
    }

    return destination;
}

void *memory_set(void *destination, int value, size_t count) {
    uint8_t *dest = (uint8_t *)destination;

    for (size_t index = 0; index < count; ++index) {
        dest[index] = (uint8_t)value;
    }

    return destination;
}

int memory_compare(const void *left, const void *right, size_t count) {
    const uint8_t *lhs = (const uint8_t *)left;
    const uint8_t *rhs = (const uint8_t *)right;

    for (size_t index = 0; index < count; ++index) {
        if (lhs[index] != rhs[index]) {
            return (int)(lhs[index] - rhs[index]);
        }
    }

    return 0;
}

size_t string_copy(char *destination, const char *source, size_t capacity) {
    size_t index = 0;

    if (capacity == 0u) {
        return 0u;
    }

    while (source[index] != '\0' && index + 1u < capacity) {
        destination[index] = source[index];
        ++index;
    }

    destination[index] = '\0';
    return index;
}
