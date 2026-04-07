#include "terminal.h"

#include "strings.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t *)0xB8000)

enum {
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,
    VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8,
    VGA_LIGHT_BLUE = 9,
    VGA_LIGHT_GREEN = 10,
    VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_LIGHT_BROWN = 14,
    VGA_WHITE = 15
};

static size_t terminal_row = 0;
static size_t terminal_column = 0;
static uint8_t terminal_color = 0;
static size_t reserved_rows = 0;

static uint16_t vga_entry(unsigned char character, uint8_t color) {
    return (uint16_t)character | ((uint16_t)color << 8);
}

static uint8_t vga_color(uint8_t foreground, uint8_t background) {
    return (uint8_t)(foreground | (background << 4));
}

static void terminal_put_entry_at(char character, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    VGA_MEMORY[index] = vga_entry((unsigned char)character, color);
}

static void terminal_set_cursor(size_t row, size_t column) {
    terminal_row = row;
    terminal_column = column;
}

static void terminal_fill_line(size_t row, uint8_t color) {
    for (size_t column = 0; column < VGA_WIDTH; ++column) {
        terminal_put_entry_at(' ', color, column, row);
    }
}

static void terminal_write_at(size_t row, size_t column, uint8_t color, const char *text) {
    while (*text != '\0' && column < VGA_WIDTH) {
        terminal_put_entry_at(*text, color, column, row);
        ++text;
        ++column;
    }
}

static void terminal_scroll(void) {
    for (size_t row = reserved_rows; row < VGA_HEIGHT - 1; ++row) {
        for (size_t column = 0; column < VGA_WIDTH; ++column) {
            VGA_MEMORY[row * VGA_WIDTH + column] =
                VGA_MEMORY[(row + 1) * VGA_WIDTH + column];
        }
    }

    terminal_fill_line(VGA_HEIGHT - 1, terminal_color);
    terminal_row = VGA_HEIGHT - 1;
    terminal_column = 0;
}

static void terminal_newline(void) {
    terminal_column = 0;
    ++terminal_row;
    if (terminal_row >= VGA_HEIGHT) {
        terminal_scroll();
    }
}

static void terminal_draw_header(void) {
    const uint8_t bar_color = vga_color(VGA_WHITE, VGA_BLUE);
    const uint8_t brand_color = vga_color(VGA_LIGHT_CYAN, VGA_BLACK);
    const uint8_t accent_color = vga_color(VGA_LIGHT_BROWN, VGA_BLACK);
    const uint8_t copy_color = vga_color(VGA_LIGHT_GREY, VGA_BLACK);

    reserved_rows = 6;

    terminal_fill_line(0, bar_color);
    terminal_write_at(0, 2, bar_color, "SNSX ExOS :: Sovereign Neural Systems Experience Operating System");

    terminal_fill_line(1, terminal_color);
    terminal_write_at(1, 2, brand_color, "+----------------------------------------------------------------------------+");
    terminal_fill_line(2, terminal_color);
    terminal_write_at(2, 2, accent_color, "| Kernel : Aurora Core x64 0.1  | Boot : Custom BIOS chain | UI : Aurora TTY |");
    terminal_fill_line(3, terminal_color);
    terminal_write_at(3, 2, copy_color, "| Commands: help about system apps learn drivers network roadmap clear reboot |");
    terminal_fill_line(4, terminal_color);
    terminal_write_at(4, 2, brand_color, "| Vision  : Custom kernel, native shell, serial logs, clean-room expansion    |");
    terminal_fill_line(5, terminal_color);
    terminal_write_at(5, 2, brand_color, "+----------------------------------------------------------------------------+");

    terminal_set_cursor(reserved_rows, 0);
}

static void terminal_write_nibble(uint8_t value) {
    const char *digits = "0123456789ABCDEF";
    terminal_write_char(digits[value & 0x0F]);
}

void terminal_init(void) {
    terminal_color = vga_color(VGA_LIGHT_GREY, VGA_BLACK);
    reserved_rows = 0;
    terminal_clear();
    terminal_draw_header();
}

void terminal_clear(void) {
    for (size_t row = 0; row < VGA_HEIGHT; ++row) {
        terminal_fill_line(row, terminal_color);
    }
    terminal_set_cursor(0, 0);
}

void terminal_clear_shell(void) {
    for (size_t row = reserved_rows; row < VGA_HEIGHT; ++row) {
        terminal_fill_line(row, terminal_color);
    }
    terminal_set_cursor(reserved_rows, 0);
}

void terminal_write_char(char character) {
    if (character == '\n') {
        terminal_newline();
        return;
    }

    if (character == '\r') {
        terminal_column = 0;
        return;
    }

    if (terminal_column >= VGA_WIDTH) {
        terminal_newline();
    }

    terminal_put_entry_at(character, terminal_color, terminal_column, terminal_row);
    ++terminal_column;
}

void terminal_write(const char *text) {
    while (*text != '\0') {
        terminal_write_char(*text);
        ++text;
    }
}

void terminal_writeln(const char *text) {
    terminal_write(text);
    terminal_write_char('\n');
}

void terminal_backspace(void) {
    if (terminal_column == 0) {
        return;
    }

    --terminal_column;
    terminal_put_entry_at(' ', terminal_color, terminal_column, terminal_row);
}

void terminal_prompt(void) {
    const uint8_t prompt_color = vga_color(VGA_LIGHT_CYAN, VGA_BLACK);
    const char *prompt = "snsx> ";

    while (*prompt != '\0') {
        terminal_put_entry_at(*prompt, prompt_color, terminal_column, terminal_row);
        ++prompt;
        ++terminal_column;
    }
}

void terminal_write_hex8(uint8_t value) {
    terminal_write_nibble((uint8_t)(value >> 4));
    terminal_write_nibble(value);
}

void terminal_write_hex32(uint32_t value) {
    terminal_write("0x");
    terminal_write_hex8((uint8_t)(value >> 24));
    terminal_write_hex8((uint8_t)(value >> 16));
    terminal_write_hex8((uint8_t)(value >> 8));
    terminal_write_hex8((uint8_t)value);
}

void terminal_write_uint(uint32_t value) {
    char buffer[11];
    size_t index = 0;

    if (value == 0) {
        terminal_write_char('0');
        return;
    }

    while (value > 0) {
        buffer[index++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    while (index > 0) {
        --index;
        terminal_write_char(buffer[index]);
    }
}

void terminal_write_uint64(uint64_t value) {
    char buffer[21];
    size_t index = 0;

    if (value == 0u) {
        terminal_write_char('0');
        return;
    }

    while (value > 0u) {
        buffer[index++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    while (index > 0u) {
        --index;
        terminal_write_char(buffer[index]);
    }
}
