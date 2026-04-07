#include "shell.h"

#include "idt.h"
#include "keyboard.h"
#include "memory.h"
#include "pit.h"
#include "runtime.h"
#include "serial.h"
#include "strings.h"
#include "terminal.h"

static const BootInfo *active_boot_info = (const BootInfo *)0;

static void cpu_halt(void) {
    __asm__ volatile("hlt");
}

static void shell_print_help(void) {
    terminal_writeln("help      Show built-in commands");
    terminal_writeln("about     Describe SNSX ExOS");
    terminal_writeln("system    Print boot and kernel facts");
    terminal_writeln("memory    Show memory manager status");
    terminal_writeln("storage   Show disk and app volume status");
    terminal_writeln("apps      List native SNSX app packages");
    terminal_writeln("launch X  Load and run an ELF app from disk");
    terminal_writeln("uptime    Show timer ticks since boot");
    terminal_writeln("services  Show active kernel subsystems");
    terminal_writeln("learn     Show the education-platform integration vision");
    terminal_writeln("drivers   Show current and planned hardware support");
    terminal_writeln("network   Show networking and Wi-Fi roadmap");
    terminal_writeln("roadmap   Show the next engineering phases");
    terminal_writeln("clear     Reset the shell area");
    terminal_writeln("reboot    Restart the machine");
    terminal_writeln("echo ...  Print text back");
}

static void shell_print_about(void) {
    terminal_writeln("SNSX ExOS is a clean-room operating-system foundation built inside this repo.");
    terminal_writeln("It boots through a custom sector chain, enters 64-bit long mode, and runs an");
    terminal_writeln("Aurora-branded terminal shell on top of a custom kernel instead of Linux.");
}

static void shell_print_system(void) {
    terminal_write("Boot mode        : ");
    terminal_writeln("Custom BIOS path");
    terminal_write("Kernel identity  : ");
    terminal_writeln("Aurora Core x64 0.1");
    terminal_write("Mapped memory    : ");
    if (active_boot_info != (const BootInfo *)0) {
        terminal_write_uint(active_boot_info->mapped_memory_mb);
        terminal_writeln(" MiB");
    } else {
        terminal_writeln("unknown");
    }

    if (active_boot_info != (const BootInfo *)0 && active_boot_info->magic == SNSX_BOOTINFO_MAGIC) {
        terminal_write("Boot drive       : ");
        terminal_write_hex8(active_boot_info->boot_drive);
        terminal_write_char('\n');

        terminal_write("Stage2 sectors   : ");
        terminal_write_uint(active_boot_info->stage2_sectors);
        terminal_write_char('\n');

        terminal_write("Kernel sectors   : ");
        terminal_write_uint(active_boot_info->kernel_sectors);
        terminal_write_char('\n');

        terminal_write("Boot flags       : ");
        terminal_write_hex32(active_boot_info->flags);
        terminal_write_char('\n');

        terminal_write("AppFS LBA        : ");
        terminal_write_uint(active_boot_info->appfs_lba);
        terminal_write_char('\n');

        terminal_write("AppFS sectors    : ");
        terminal_write_uint(active_boot_info->appfs_sectors);
        terminal_write_char('\n');
    } else {
        terminal_writeln("Boot info        : unavailable");
    }
}

static void shell_print_memory(void) {
    terminal_write("Total memory     : ");
    terminal_write_uint64(memory_total_bytes() / 1024u);
    terminal_writeln(" KiB");
    terminal_write("Used memory      : ");
    terminal_write_uint64(memory_used_bytes() / 1024u);
    terminal_writeln(" KiB");
    terminal_write("Free memory      : ");
    terminal_write_uint64(memory_free_bytes() / 1024u);
    terminal_writeln(" KiB");
    terminal_write("Kernel heap      : ");
    terminal_write_uint64(memory_heap_bytes() / 1024u);
    terminal_writeln(" KiB");
    terminal_write("Heap free        : ");
    terminal_write_uint64(memory_heap_free_bytes() / 1024u);
    terminal_writeln(" KiB");
}

static void shell_print_storage(void) {
    terminal_writeln("Disk subsystem   : ATA PIO block driver");
    terminal_write("App volume LBA   : ");
    terminal_write_uint(runtime_storage_lba());
    terminal_write_char('\n');
    terminal_write("App volume secs  : ");
    terminal_write_uint(runtime_storage_sectors());
    terminal_write_char('\n');
    terminal_write("Catalog entries  : ");
    terminal_write_uint(runtime_app_count());
    terminal_write_char('\n');
}

static void shell_print_apps(void) {
    if (!runtime_is_ready()) {
        terminal_writeln("Native app catalog is not available.");
        return;
    }

    terminal_writeln("Installed SNSX applications:");
    for (uint32_t index = 0; index < runtime_app_count(); ++index) {
        terminal_write("- ");
        terminal_write(runtime_app_name(index));
        terminal_write(" (");
        terminal_write_uint(runtime_app_size(index));
        terminal_writeln(" bytes)");
    }
}

static void shell_print_uptime(void) {
    terminal_write("Ticks            : ");
    terminal_write_uint64(pit_ticks());
    terminal_write_char('\n');
    terminal_write("Timer frequency  : ");
    terminal_write_uint(pit_frequency_hz());
    terminal_writeln(" Hz");
}

static void shell_print_services(void) {
    terminal_writeln("Aurora Terminal  : online");
    terminal_writeln("Memory Manager   : page allocator + heap");
    terminal_writeln("PIT Timer        : online");
    terminal_writeln("ATA Storage      : online");
    terminal_writeln("AppFS Catalog    : online");
    terminal_writeln("ELF Runtime      : online");
    terminal_writeln("Input Service    : PS/2 keyboard online");
}

static void shell_print_learn(void) {
    terminal_writeln("EduVerse Bridge vision:");
    terminal_writeln("1. Native file and package layer for lessons, notes, and local AI modules.");
    terminal_writeln("2. Native app runtime for teacher, student, and admin experiences.");
    terminal_writeln("3. Graphics compositor and audio stack for immersive education tools.");
}

static void shell_print_drivers(void) {
    terminal_writeln("Working now:");
    terminal_writeln("- BIOS disk bootstrap");
    terminal_writeln("- VGA text console");
    terminal_writeln("- PS/2 keyboard input");
    terminal_writeln("- PIT hardware timer");
    terminal_writeln("- Serial logging on COM1");
    terminal_writeln("- ATA PIO boot-disk access");
    terminal_writeln("Planned next:");
    terminal_writeln("- PCI discovery and storage drivers");
    terminal_writeln("- USB host stack");
    terminal_writeln("- VirtIO and e1000 networking");
    terminal_writeln("- Wi-Fi and Bluetooth controller support");
}

static void shell_print_network(void) {
    terminal_writeln("Networking in a real custom OS takes several layers:");
    terminal_writeln("- PCI or platform driver discovery");
    terminal_writeln("- Ethernet or Wi-Fi device drivers");
    terminal_writeln("- Packet buffers, ARP, IPv4, ICMP, UDP, TCP");
    terminal_writeln("- DNS, sockets, security, and wireless management");
    terminal_writeln("That stack is architected as future work, not falsely claimed as complete.");
}

static void shell_print_roadmap(void) {
    terminal_writeln("Phase 1: process model, syscall boundary, and stronger isolation");
    terminal_writeln("Phase 2: VFS, package evolution, and richer native apps");
    terminal_writeln("Phase 3: GUI compositor, windowing, input, audio, and theming");
    terminal_writeln("Phase 4: Ethernet, Wi-Fi, Bluetooth, and internet-facing services");
    terminal_writeln("Phase 5: port the education platform into native SNSX ExOS applications");
}

static void shell_reboot(void) {
    interrupts_disable();
    __asm__ volatile("int $0x19");
    for (;;) {
        cpu_halt();
    }
}

static void shell_execute(const char *command) {
    if (string_compare(command, "help") == 0) {
        shell_print_help();
        return;
    }

    if (string_compare(command, "about") == 0) {
        shell_print_about();
        return;
    }

    if (string_compare(command, "system") == 0) {
        shell_print_system();
        return;
    }

    if (string_compare(command, "memory") == 0) {
        shell_print_memory();
        return;
    }

    if (string_compare(command, "storage") == 0) {
        shell_print_storage();
        return;
    }

    if (string_compare(command, "apps") == 0) {
        shell_print_apps();
        return;
    }

    if (string_compare(command, "uptime") == 0) {
        shell_print_uptime();
        return;
    }

    if (string_compare(command, "services") == 0) {
        shell_print_services();
        return;
    }

    if (string_compare(command, "learn") == 0) {
        shell_print_learn();
        return;
    }

    if (string_compare(command, "drivers") == 0) {
        shell_print_drivers();
        return;
    }

    if (string_compare(command, "network") == 0) {
        shell_print_network();
        return;
    }

    if (string_compare(command, "roadmap") == 0) {
        shell_print_roadmap();
        return;
    }

    if (string_compare(command, "clear") == 0) {
        terminal_init();
        return;
    }

    if (string_compare(command, "reboot") == 0) {
        shell_reboot();
        return;
    }

    if (string_starts_with(command, "launch ")) {
        if (runtime_launch(command + 7)) {
            terminal_write("App exit code    : ");
            terminal_write_uint(runtime_last_exit_code());
            terminal_write_char('\n');
        } else {
            terminal_writeln("Launch failed. Check the app name with 'apps'.");
        }
        return;
    }

    if (string_starts_with(command, "echo ")) {
        terminal_writeln(command + 5);
        return;
    }

    terminal_writeln("Unknown command. Type 'help' for the command list.");
}

void shell_run(const BootInfo *boot_info) {
    char input_buffer[128];
    size_t input_length = 0;

    active_boot_info = boot_info;

    terminal_writeln("SNSX ExOS is ready.");
    terminal_writeln("Type 'help' to explore the current kernel features.");
    terminal_writeln("");
    terminal_prompt();

    for (;;) {
        const char character = keyboard_pop();
        if (character == '\0') {
            cpu_halt();
            continue;
        }

        if (character == '\b') {
            if (input_length > 0) {
                --input_length;
                terminal_backspace();
            }
            continue;
        }

        if (character == '\n') {
            input_buffer[input_length] = '\0';
            terminal_write_char('\n');

            if (input_length > 0) {
                serial_write("[SNSX] command: ");
                serial_writeln(input_buffer);
                shell_execute(input_buffer);
            }

            input_length = 0;
            terminal_prompt();
            continue;
        }

        if (input_length + 1u >= sizeof(input_buffer)) {
            continue;
        }

        input_buffer[input_length++] = character;
        terminal_write_char(character);
    }
}
