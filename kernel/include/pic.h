#ifndef SNSX_PIC_H
#define SNSX_PIC_H

#include "types.h"

void pic_remap(void);
void pic_set_masks(uint8_t master_mask, uint8_t slave_mask);
void pic_send_eoi(uint8_t irq);

#endif
