#include "idt.h"

#include "types.h"

extern void default_interrupt_stub(void);
extern void timer_interrupt_stub(void);
extern void keyboard_interrupt_stub(void);
extern void idt_load(const void *pointer);

typedef struct IdtEntry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attributes;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) IdtEntry;

typedef struct IdtPointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) IdtPointer;

static IdtEntry idt[256];

static void idt_set_gate(uint8_t vector, void (*handler)(void)) {
    const uint64_t address = (uint64_t)handler;

    idt[vector].offset_low = (uint16_t)(address & 0xFFFFu);
    idt[vector].selector = 0x18;
    idt[vector].ist = 0;
    idt[vector].type_attributes = 0x8E;
    idt[vector].offset_mid = (uint16_t)((address >> 16) & 0xFFFFu);
    idt[vector].offset_high = (uint32_t)(address >> 32);
    idt[vector].reserved = 0;
}

void idt_init(void) {
    for (uint16_t vector = 0; vector < 256u; ++vector) {
        idt_set_gate((uint8_t)vector, default_interrupt_stub);
    }

    idt_set_gate(32, timer_interrupt_stub);
    idt_set_gate(33, keyboard_interrupt_stub);

    const IdtPointer pointer = {
        .limit = (uint16_t)(sizeof(idt) - 1u),
        .base = (uint64_t)&idt[0]
    };

    idt_load(&pointer);
}

void interrupt_default_handler(void) {
}
