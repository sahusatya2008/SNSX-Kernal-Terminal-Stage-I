#ifndef SNSX_TERMINAL_H
#define SNSX_TERMINAL_H

#include "types.h"

void terminal_init(void);
void terminal_clear(void);
void terminal_clear_shell(void);
void terminal_write_char(char character);
void terminal_write(const char *text);
void terminal_writeln(const char *text);
void terminal_backspace(void);
void terminal_prompt(void);
void terminal_write_hex8(uint8_t value);
void terminal_write_hex32(uint32_t value);
void terminal_write_uint(uint32_t value);
void terminal_write_uint64(uint64_t value);

#endif
