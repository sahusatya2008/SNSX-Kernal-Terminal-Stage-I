#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="$ROOT/dist/snsx-exos.img"
QEMU_BIN="${QEMU_BIN:-qemu-system-x86_64}"
mkdir -p "$ROOT/build/tmp"
export TMPDIR="$ROOT/build/tmp"

if [[ ! -f "$IMAGE" ]]; then
  "$ROOT/scripts/build.sh"
fi

COMMON_ARGS=(
  -object memory-backend-ram,id=mem,size=256M
  -machine pc,memory-backend=mem
  -boot c
  -drive "format=raw,file=$IMAGE"
  -serial stdio
  -monitor none
  -no-reboot
)

if [[ "${SNSX_HEADLESS:-0}" == "1" ]]; then
  exec "$QEMU_BIN" "${COMMON_ARGS[@]}" -display none
fi

exec "$QEMU_BIN" "${COMMON_ARGS[@]}"
