#include "pit.h"

#include "pic.h"
#include "ports.h"

#define PIT_BASE_FREQUENCY 1193182u

static volatile uint64_t current_ticks = 0;
static uint32_t configured_frequency_hz = 100;

void pit_init(uint32_t frequency_hz) {
    if (frequency_hz == 0u) {
        frequency_hz = 100u;
    }

    configured_frequency_hz = frequency_hz;
    const uint16_t divisor = (uint16_t)(PIT_BASE_FREQUENCY / frequency_hz);

    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFFu));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFFu));
}

void pit_irq_handler(void) {
    ++current_ticks;
    pic_send_eoi(0);
}

uint64_t pit_ticks(void) {
    return current_ticks;
}

uint32_t pit_frequency_hz(void) {
    return configured_frequency_hz;
}
