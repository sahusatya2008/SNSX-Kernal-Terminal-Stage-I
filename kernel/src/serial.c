#include "serial.h"

#include "ports.h"

#define COM1_PORT 0x3F8

static void serial_write_nibble(uint8_t value) {
    const char *digits = "0123456789ABCDEF";
    serial_write_char(digits[value & 0x0F]);
}

void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT + 0, 0x03);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xC7);
    outb(COM1_PORT + 4, 0x0B);
}

void serial_write_char(char character) {
    while ((inb(COM1_PORT + 5) & 0x20u) == 0u) {
    }
    outb(COM1_PORT, (uint8_t)character);
}

void serial_write(const char *text) {
    while (*text != '\0') {
        if (*text == '\n') {
            serial_write_char('\r');
        }
        serial_write_char(*text);
        ++text;
    }
}

void serial_writeln(const char *text) {
    serial_write(text);
    serial_write("\n");
}

void serial_write_hex8(uint8_t value) {
    serial_write_nibble((uint8_t)(value >> 4));
    serial_write_nibble(value);
}

void serial_write_hex32(uint32_t value) {
    serial_write("0x");
    serial_write_hex8((uint8_t)(value >> 24));
    serial_write_hex8((uint8_t)(value >> 16));
    serial_write_hex8((uint8_t)(value >> 8));
    serial_write_hex8((uint8_t)value);
}

void serial_write_uint64(uint64_t value) {
    char buffer[21];
    size_t index = 0;

    if (value == 0u) {
        serial_write_char('0');
        return;
    }

    while (value > 0u) {
        buffer[index++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    while (index > 0u) {
        --index;
        serial_write_char(buffer[index]);
    }
}
