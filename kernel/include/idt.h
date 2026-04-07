#ifndef SNSX_IDT_H
#define SNSX_IDT_H

#include "types.h"

void idt_init(void);
void interrupts_enable(void);
void interrupts_disable(void);
void interrupt_default_handler(void);
void pit_irq_handler(void);

#endif
