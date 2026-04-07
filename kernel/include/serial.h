#ifndef SNSX_SERIAL_H
#define SNSX_SERIAL_H

#include "types.h"

void serial_init(void);
void serial_write_char(char character);
void serial_write(const char *text);
void serial_writeln(const char *text);
void serial_write_hex8(uint8_t value);
void serial_write_hex32(uint32_t value);
void serial_write_uint64(uint64_t value);

#endif
