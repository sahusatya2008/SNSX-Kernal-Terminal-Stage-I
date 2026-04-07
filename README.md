# SNSX ExOS

`SNSX ExOS` is a real clean-room operating-system foundation added beside the existing education platform. It does **not** use Linux or Windows as its kernel. Instead, it boots through a custom BIOS boot sector, loads a custom second-stage loader, switches into 64-bit long mode, and starts a custom kernel with its own terminal shell.

## What this build delivers

- Custom stage-1 boot sector
- Custom stage-2 loader
- Custom 64-bit kernel (`Aurora Core x64`)
- VGA text-mode UI with branded `Aurora Terminal`
- PS/2 keyboard input
- PIT timer and uptime tracking
- Kernel page allocator and heap
- ATA boot-disk storage access
- Disk-backed `AppFS` package volume
- ELF executable loading
- Native SNSX application runtime
- Packaged sample app (`aurora-demo`)
- COM1 serial logging for debugging and headless verification
- Bootable raw disk image for USB writing on legacy BIOS-compatible systems
- `VirtualBox`-friendly `VDI` image

## What it does not falsely claim yet

This is **not** already a full commercial-scale operating system with:

- Wi-Fi drivers
- Bluetooth stack
- Modern GPU drivers
- General-purpose filesystem and package manager
- Strong process isolation and protected user mode
- GUI compositor and window manager
- Browser, installer, updater, or app ecosystem

Those systems require substantial additional engineering and are laid out in the roadmap docs instead of being misrepresented as finished.

## Build

```bash
cd "/Volumes/Blockchain Drive/HighLevelSoftwares/Incomplete Project/education/snsx-exos"
make
```

Artifacts:

- `dist/snsx-exos.img`
- `dist/snsx-exos.vdi`

## Run in QEMU

```bash
make run
```

For headless serial-only verification:

```bash
make headless
```

The headless boot log now verifies:

- boot sector and long-mode handoff
- memory manager startup
- disk-backed app catalog load
- ELF app validation and loading
- native app execution and clean return

## Run in VirtualBox

1. Create a new `Other/Unknown (64-bit)` VM.
2. Attach [`dist/snsx-exos.vdi`](/Volumes/Blockchain%20Drive/HighLevelSoftwares/Incomplete%20Project/education/snsx-exos/dist/snsx-exos.vdi) as the primary disk.
3. Boot the VM.

## Write to a USB drive

This image currently targets **legacy BIOS-compatible** booting.

Example:

```bash
sudo dd if=dist/snsx-exos.img of=/dev/rdiskN bs=1m
```

Replace `/dev/rdiskN` with the correct target device.

## Docs

- [Architecture](/Volumes/Blockchain Drive/HighLevelSoftwares/Incomplete Project/education/snsx-exos/docs/ARCHITECTURE.md)
- [Roadmap](/Volumes/Blockchain Drive/HighLevelSoftwares/Incomplete Project/education/snsx-exos/docs/ROADMAP.md)
