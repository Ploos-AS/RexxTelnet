#!/usr/bin/env bash
set -euo pipefail

IMAGE="${REXXTELNET_BEBBO_IMAGE:-amigadev/m68k-amigaos-gcc@sha256:b18080e6ffca8f793e0f539536a9138e9d2a548ca1a301c7483f43ee15fedfed}"
OUT_DIR="${1:-build/fs-uae/native}"
mkdir -p "$OUT_DIR"

docker pull "$IMAGE"
docker image inspect "$IMAGE" --format '{{join .RepoDigests "\n"}}' | tee "$OUT_DIR/toolchain-image.txt"

docker run --rm \
  -v "$PWD:/work" \
  -w /work \
  "$IMAGE" \
  m68k-amigaos-gcc \
    -Isrc \
    -Os -Wall -Wextra -Werror -m68000 \
    -noixemul \
    -o RexxTelnet \
    src/main.c \
    src/amiga_runtime.c \
    src/app_session.c \
    src/arexx_amiga.c \
    src/arexx_cmd.c \
    src/arexx_dispatch.c \
    src/rx_buffer.c \
    src/session.c \
    src/telnet.c \
    src/terminal.c \
    src/terminal_amiga.c \
    src/transport_common.c \
    src/transport_amiga.c

cp RexxTelnet "$OUT_DIR/RexxTelnet"
file "$OUT_DIR/RexxTelnet" | tee "$OUT_DIR/file.txt"
sha256sum "$OUT_DIR/RexxTelnet" | tee "$OUT_DIR/RexxTelnet.sha256"

if ! grep -Eiq 'AmigaOS|Amiga.*executable|loadseg' "$OUT_DIR/file.txt"; then
  echo "ERROR: native output is not recognized as an Amiga executable" >&2
  exit 1
fi

printf 'STATUS=PASS\nGATE=M5_1_NATIVE_BEBBO_BUILD\nIMAGE=%s\nBINARY=%s\n' "$IMAGE" "$OUT_DIR/RexxTelnet" | tee "$OUT_DIR/result.txt"
