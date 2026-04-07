#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build"
BOOT_BUILD_DIR="$BUILD_DIR/boot"
KERNEL_BUILD_DIR="$BUILD_DIR/kernel"
APP_BUILD_DIR="$BUILD_DIR/apps"
TOOLS_BUILD_DIR="$BUILD_DIR/tools"
DIST_DIR="$ROOT/dist"

CC="${CC:-clang}"
HOST_CC="${HOST_CC:-clang}"
LD="${LD:-ld.lld}"
ASM="${ASM:-nasm}"
QEMU_IMG="${QEMU_IMG:-qemu-img}"

mkdir -p "$BOOT_BUILD_DIR" "$KERNEL_BUILD_DIR" "$APP_BUILD_DIR" "$TOOLS_BUILD_DIR" "$DIST_DIR"
mkdir -p "$BUILD_DIR/tmp"
export TMPDIR="$BUILD_DIR/tmp"

CFLAGS=(
  -target x86_64-unknown-none-elf
  -ffreestanding
  -fno-builtin
  -fno-stack-protector
  -fno-pic
  -mno-red-zone
  -nostdlib
  -Wall
  -Wextra
  -Werror
  -std=c11
  -I"$ROOT/kernel/include"
  -I"$ROOT/shared"
)

KERNEL_C_SOURCES=(
  main.c
  terminal.c
  serial.c
  ports.c
  pic.c
  keyboard.c
  idt.c
  pit.c
  memory.c
  ata.c
  appfs.c
  runtime.c
  shell.c
  strings.c
)

KERNEL_OBJECTS=(
  "$KERNEL_BUILD_DIR/entry.o"
  "$KERNEL_BUILD_DIR/interrupts.o"
)

for source in "${KERNEL_C_SOURCES[@]}"; do
  object_name="${source%.c}.o"
  "$CC" "${CFLAGS[@]}" -c "$ROOT/kernel/src/$source" -o "$KERNEL_BUILD_DIR/$object_name"
  KERNEL_OBJECTS+=("$KERNEL_BUILD_DIR/$object_name")
done

"$ASM" -f elf64 "$ROOT/kernel/entry.asm" -o "$KERNEL_BUILD_DIR/entry.o"
"$ASM" -f elf64 "$ROOT/kernel/interrupts.asm" -o "$KERNEL_BUILD_DIR/interrupts.o"

"$LD" -T "$ROOT/kernel/linker.ld" --oformat binary "${KERNEL_OBJECTS[@]}" -o "$KERNEL_BUILD_DIR/kernel.bin"

APP_CFLAGS=(
  -target x86_64-unknown-none-elf
  -ffreestanding
  -fno-builtin
  -fno-stack-protector
  -fno-pic
  -mno-red-zone
  -nostdlib
  -Wall
  -Wextra
  -Werror
  -std=c11
  -I"$ROOT/shared"
)

"$CC" "${APP_CFLAGS[@]}" -c "$ROOT/apps/demo/main.c" -o "$APP_BUILD_DIR/aurora-demo.o"
"$LD" -T "$ROOT/apps/linker.ld" -z max-page-size=0x1000 "$APP_BUILD_DIR/aurora-demo.o" -o "$APP_BUILD_DIR/aurora-demo.elf"

"$HOST_CC" -std=c11 -Wall -Wextra -Werror "$ROOT/tools/mkappfs.c" -o "$TOOLS_BUILD_DIR/mkappfs"
"$TOOLS_BUILD_DIR/mkappfs" "$BUILD_DIR/appfs.bin" aurora-demo "$APP_BUILD_DIR/aurora-demo.elf"

kernel_size="$(stat -f%z "$KERNEL_BUILD_DIR/kernel.bin")"
kernel_sectors="$(((kernel_size + 511) / 512))"
if (( kernel_sectors == 0 )); then
  kernel_sectors=1
fi

appfs_size="$(stat -f%z "$BUILD_DIR/appfs.bin")"
appfs_sectors="$(((appfs_size + 511) / 512))"
if (( appfs_sectors == 0 )); then
  appfs_sectors=1
fi

"$ASM" -f bin "$ROOT/boot/stage2.asm" -o "$BOOT_BUILD_DIR/stage2.probe.bin" \
  -DKERNEL_LBA=2 \
  -DKERNEL_SECTORS="$kernel_sectors" \
  -DAPPFS_LBA=3 \
  -DAPPFS_SECTORS="$appfs_sectors" \
  -DMAPPED_MEMORY_MB=64 \
  -DSTAGE2_SECTORS=1

stage2_probe_size="$(stat -f%z "$BOOT_BUILD_DIR/stage2.probe.bin")"
stage2_sectors="$(((stage2_probe_size + 511) / 512))"
if (( stage2_sectors == 0 )); then
  stage2_sectors=1
fi

"$ASM" -f bin "$ROOT/boot/stage2.asm" -o "$BOOT_BUILD_DIR/stage2.bin" \
  -DKERNEL_LBA="$((1 + stage2_sectors))" \
  -DKERNEL_SECTORS="$kernel_sectors" \
  -DAPPFS_LBA="$((1 + stage2_sectors + kernel_sectors))" \
  -DAPPFS_SECTORS="$appfs_sectors" \
  -DMAPPED_MEMORY_MB=64 \
  -DSTAGE2_SECTORS="$stage2_sectors"

final_stage2_size="$(stat -f%z "$BOOT_BUILD_DIR/stage2.bin")"
final_stage2_sectors="$(((final_stage2_size + 511) / 512))"

if (( final_stage2_sectors != stage2_sectors )); then
  stage2_sectors="$final_stage2_sectors"
  "$ASM" -f bin "$ROOT/boot/stage2.asm" -o "$BOOT_BUILD_DIR/stage2.bin" \
    -DKERNEL_LBA="$((1 + stage2_sectors))" \
    -DKERNEL_SECTORS="$kernel_sectors" \
    -DAPPFS_LBA="$((1 + stage2_sectors + kernel_sectors))" \
    -DAPPFS_SECTORS="$appfs_sectors" \
    -DMAPPED_MEMORY_MB=64 \
    -DSTAGE2_SECTORS="$stage2_sectors"
fi

"$ASM" -f bin "$ROOT/boot/stage1.asm" -o "$BOOT_BUILD_DIR/stage1.bin" -DSTAGE2_SECTORS="$stage2_sectors"

stage1_size="$(stat -f%z "$BOOT_BUILD_DIR/stage1.bin")"
if (( stage1_size != 512 )); then
  echo "stage1 must be exactly 512 bytes, got $stage1_size" >&2
  exit 1
fi

RAW_IMAGE="$DIST_DIR/snsx-exos.img"
VDI_IMAGE="$DIST_DIR/snsx-exos.vdi"

rm -f "$RAW_IMAGE" "$VDI_IMAGE"
dd if=/dev/zero of="$RAW_IMAGE" bs=1048576 count=32 >/dev/null 2>&1
dd if="$BOOT_BUILD_DIR/stage1.bin" of="$RAW_IMAGE" bs=512 seek=0 conv=notrunc >/dev/null 2>&1
dd if="$BOOT_BUILD_DIR/stage2.bin" of="$RAW_IMAGE" bs=512 seek=1 conv=notrunc >/dev/null 2>&1
dd if="$KERNEL_BUILD_DIR/kernel.bin" of="$RAW_IMAGE" bs=512 seek="$((1 + stage2_sectors))" conv=notrunc >/dev/null 2>&1
dd if="$BUILD_DIR/appfs.bin" of="$RAW_IMAGE" bs=512 seek="$((1 + stage2_sectors + kernel_sectors))" conv=notrunc >/dev/null 2>&1

"$QEMU_IMG" convert -f raw -O vdi "$RAW_IMAGE" "$VDI_IMAGE" >/dev/null

printf '%s\n' \
  "SNSX ExOS build complete" \
  "  Raw image : $RAW_IMAGE" \
  "  VDI image : $VDI_IMAGE" \
  "  Stage2    : $stage2_sectors sector(s)" \
  "  Kernel    : $kernel_sectors sector(s)" \
  "  AppFS     : $appfs_sectors sector(s)"
