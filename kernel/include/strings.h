#ifndef SNSX_STRINGS_H
#define SNSX_STRINGS_H

#include "types.h"

size_t string_length(const char *text);
int string_compare(const char *left, const char *right);
int string_ncompare(const char *left, const char *right, size_t count);
bool string_starts_with(const char *text, const char *prefix);
void *memory_copy(void *destination, const void *source, size_t count);
void *memory_set(void *destination, int value, size_t count);
int memory_compare(const void *left, const void *right, size_t count);
size_t string_copy(char *destination, const char *source, size_t capacity);

#endif
