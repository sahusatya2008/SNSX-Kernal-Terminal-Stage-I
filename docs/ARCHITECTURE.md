# SNSX ExOS Architecture

## Current boot path

1. BIOS loads `stage1` at `0x7C00`.
2. `stage1` uses BIOS extended disk reads to load `stage2`.
3. `stage2` enables A20, reads the kernel into memory, installs a GDT, enables PAE paging, sets `EFER.LME`, and jumps into 64-bit long mode.
4. The kernel starts at `0x20000`, clears `.bss`, initializes serial, terminal, IDT, PIC, keyboard input, and the shell.

## Current kernel modules

- `terminal.c`: VGA text UI and shell rendering
- `serial.c`: COM1 debugging output for headless verification
- `pic.c`: Legacy PIC remapping and IRQ signaling
- `keyboard.c`: PS/2 scancode translation and input queue
- `idt.c`: Interrupt descriptor table setup
- `pit.c`: hardware timer and uptime ticks
- `memory.c`: page allocator and kernel heap
- `ata.c`: PIO-based access to the boot disk
- `appfs.c`: read-only packaged app volume
- `runtime.c`: ELF loading and native app execution
- `shell.c`: Command interpreter and boot/runtime reporting

## Delivered system boundaries

The current build is intentionally a **foundation**:

- single address space
- no scheduler
- no protected user mode
- no general-purpose filesystem
- no network stack
- no Wi-Fi or Bluetooth drivers
- no GUI compositor

## Current runtime model

`SNSX ExOS` now includes a native app runtime:

1. host build compiles ELF applications into a custom `AppFS` volume
2. the kernel reads `AppFS` from the boot disk using the ATA driver
3. the runtime validates ELF headers and copies `PT_LOAD` segments into the reserved app window
4. the kernel calls the app entry with a small SNSX app API for terminal output, uptime, and memory metrics

This is a real application-loading path, but it is not yet protected user mode or process isolation.

## Why this structure

This gives `SNSX ExOS` a real bootable identity immediately while keeping the next systems-programming steps clean:

1. add memory management and allocation
2. add storage and a virtual filesystem
3. add executable loading and process isolation
4. add networking and wireless stacks
5. add graphics, compositor, and native applications
