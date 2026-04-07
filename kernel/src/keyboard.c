#include "keyboard.h"

#include "pic.h"
#include "ports.h"

#define KEYBOARD_BUFFER_SIZE 256

static volatile char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint8_t buffer_head = 0;
static volatile uint8_t buffer_tail = 0;
static volatile bool shift_pressed = false;

static const char keymap[128] = {
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4', [0x06] = '5',
    [0x07] = '6', [0x08] = '7', [0x09] = '8', [0x0A] = '9', [0x0B] = '0',
    [0x0C] = '-', [0x0D] = '=', [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't',
    [0x15] = 'y', [0x16] = 'u', [0x17] = 'i', [0x18] = 'o', [0x19] = 'p',
    [0x1A] = '[', [0x1B] = ']', [0x1C] = '\n',
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g',
    [0x23] = 'h', [0x24] = 'j', [0x25] = 'k', [0x26] = 'l',
    [0x27] = ';', [0x28] = '\'', [0x29] = '`',
    [0x2B] = '\\',
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v', [0x30] = 'b',
    [0x31] = 'n', [0x32] = 'm', [0x33] = ',', [0x34] = '.', [0x35] = '/',
    [0x39] = ' '
};

static const char shifted_keymap[128] = {
    [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$', [0x06] = '%',
    [0x07] = '^', [0x08] = '&', [0x09] = '*', [0x0A] = '(', [0x0B] = ')',
    [0x0C] = '_', [0x0D] = '+',
    [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E', [0x13] = 'R', [0x14] = 'T',
    [0x15] = 'Y', [0x16] = 'U', [0x17] = 'I', [0x18] = 'O', [0x19] = 'P',
    [0x1A] = '{', [0x1B] = '}', [0x1C] = '\n',
    [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D', [0x21] = 'F', [0x22] = 'G',
    [0x23] = 'H', [0x24] = 'J', [0x25] = 'K', [0x26] = 'L',
    [0x27] = ':', [0x28] = '"', [0x29] = '~',
    [0x2B] = '|',
    [0x2C] = 'Z', [0x2D] = 'X', [0x2E] = 'C', [0x2F] = 'V', [0x30] = 'B',
    [0x31] = 'N', [0x32] = 'M', [0x33] = '<', [0x34] = '>', [0x35] = '?',
    [0x39] = ' '
};

static void buffer_push(char character) {
    const uint8_t next_head = (uint8_t)(buffer_head + 1u);
    if (next_head == buffer_tail) {
        return;
    }

    keyboard_buffer[buffer_head] = character;
    buffer_head = next_head;
}

void keyboard_init(void) {
    buffer_head = 0;
    buffer_tail = 0;
    shift_pressed = false;
}

void keyboard_irq_handler(void) {
    const uint8_t scan_code = inb(0x60);

    if (scan_code == 0x2A || scan_code == 0x36) {
        shift_pressed = true;
        pic_send_eoi(1);
        return;
    }

    if (scan_code == 0xAA || scan_code == 0xB6) {
        shift_pressed = false;
        pic_send_eoi(1);
        return;
    }

    if ((scan_code & 0x80u) != 0u) {
        pic_send_eoi(1);
        return;
    }

    const char character = shift_pressed ? shifted_keymap[scan_code] : keymap[scan_code];
    if (character != '\0') {
        buffer_push(character);
    }

    pic_send_eoi(1);
}

char keyboard_pop(void) {
    if (buffer_head == buffer_tail) {
        return '\0';
    }

    const char character = keyboard_buffer[buffer_tail];
    buffer_tail = (uint8_t)(buffer_tail + 1u);
    return character;
}
