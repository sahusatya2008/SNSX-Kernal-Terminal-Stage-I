#include "boot_info.h"
#include "memory.h"
#include "idt.h"
#include "keyboard.h"
#include "pic.h"
#include "pit.h"
#include "runtime.h"
#include "serial.h"
#include "shell.h"
#include "terminal.h"

extern uint8_t __kernel_end[];

void kmain(const BootInfo *boot_info) {
    serial_init();
    serial_writeln("[SNSX] serial online");

    terminal_init();
    terminal_writeln("Aurora Terminal is live.");
    terminal_writeln("Boot pipeline: stage1 -> stage2 -> Aurora Core -> shell.");
    terminal_writeln("");

    if (boot_info != (const BootInfo *)0 && boot_info->magic == SNSX_BOOTINFO_MAGIC) {
        serial_writeln("[SNSX] boot info accepted");
    } else {
        serial_writeln("[SNSX] boot info missing or invalid");
    }

    memory_init(boot_info, (uintptr_t)__kernel_end);
    idt_init();
    pic_remap();
    pic_set_masks(0xFC, 0xFF);
    pit_init(100);
    keyboard_init();

    if (runtime_init(boot_info)) {
        serial_writeln("[SNSX] runtime catalog loaded");
    } else {
        serial_writeln("[SNSX] runtime catalog unavailable");
    }

    interrupts_enable();

    serial_writeln("[SNSX] keyboard, timer, and shell ready");
    runtime_autorun();
    shell_run(boot_info);
}
