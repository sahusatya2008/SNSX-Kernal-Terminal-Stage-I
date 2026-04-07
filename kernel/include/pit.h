#ifndef SNSX_PIT_H
#define SNSX_PIT_H

#include "types.h"

void pit_init(uint32_t frequency_hz);
void pit_irq_handler(void);
uint64_t pit_ticks(void);
uint32_t pit_frequency_hz(void);

#endif
