#ifndef SNSX_KEYBOARD_H
#define SNSX_KEYBOARD_H

#include "types.h"

void keyboard_init(void);
void keyboard_irq_handler(void);
char keyboard_pop(void);

#endif
