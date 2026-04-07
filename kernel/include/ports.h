#ifndef SNSX_PORTS_H
#define SNSX_PORTS_H

#include "types.h"

uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t value);
void io_wait(void);

#endif
