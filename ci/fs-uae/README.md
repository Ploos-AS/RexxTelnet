# FS-UAE / AROS CI harness

This harness mirrors the AmiNTP qualification pattern for RexxTelnet.

## Gate 1: internal AROS Kickstart smoke

`run-aros-smoke.sh` boots an A1200-class FS-UAE machine with `kickstart_file = internal` under Xvfb. Remaining alive for 20 seconds is the PASS condition.

## Gate 2: AROS m68k boot media

`fetch-aros-boot.sh` resolves the current official AROS amiga-m68k boot-floppy archive, records the source URL and SHA-256, extracts the ADF, and normalizes it to `build/fs-uae/aros-boot/bootdisk.adf`.

`run-aros-media-smoke.sh` boots that media with FS-UAE's internal AROS replacement ROM and requires the emulator to remain alive for 25 seconds.

## Gate 3: native Bebbo build

`build-native.sh` uses the same digest-pinned Bebbo GCC container as AmiNTP and compiles RexxTelnet for `-m68000 -noixemul`. The produced binary must be recognized as an Amiga executable. Toolchain digest, `file` output and binary SHA-256 are preserved as evidence.

## Gate 4: M5.3a AROS guest TCP runtime

`run-aros-guest-runtime.sh` downloads and extracts the current AROS m68k system image, injects the native RexxTelnet binary into the guest filesystem, enables FS-UAE `bsdsocket_library = 1`, starts RexxTelnet from the guest startup sequence, and requires a connection to a controlled host-side TCP listener.

The AROS filesystem is inspected for `RX`, `RexxMast` and `rexxsyslib.library`, but those are observational only. The currently used public AROS image does not provide the full classic ARexx runtime needed to qualify the `REXXTELNET` port, so ARexx is intentionally not part of the automated AROS PASS condition.

## M5.3b: local classic AmigaOS ARexx qualification

ARexx is qualified separately under a local licensed/classic AmigaOS FS-UAE environment with RexxMast and `rexxsyslib.library` present. The fixed procedure and PASS criteria are in `docs/M5_3_QUALIFICATION.md`, with helpers under `ci/local/`.

All automated evidence is written below `build/fs-uae/` and uploaded by the workflow.
