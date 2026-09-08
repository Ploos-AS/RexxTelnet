# FS-UAE / AROS CI harness

This harness mirrors the AmiNTP qualification pattern for RexxTelnet.

## Gate 1: internal AROS Kickstart smoke

`run-aros-smoke.sh` boots an A1200-class FS-UAE machine with `kickstart_file = internal` under Xvfb. Remaining alive for 20 seconds is the PASS condition.

## Gate 2: AROS m68k boot media

`fetch-aros-boot.sh` resolves the current official AROS amiga-m68k boot-floppy archive, records the source URL and SHA-256, extracts the ADF, and normalizes it to `build/fs-uae/aros-boot/bootdisk.adf`.

`run-aros-media-smoke.sh` boots that media with FS-UAE's internal AROS replacement ROM and requires the emulator to remain alive for 25 seconds.

## Gate 3: native Bebbo build

`build-native.sh` uses the same digest-pinned Bebbo GCC container as AmiNTP and compiles RexxTelnet for `-m68000 -noixemul`. The produced binary must be recognized as an Amiga executable. Toolchain digest, `file` output and binary SHA-256 are preserved as evidence.

These gates establish runner viability, AROS/FS-UAE boot viability and native-build viability. They do not yet prove that RexxTelnet, RexxMast, the `REXXTELNET` ARexx port, bsdsocket networking, Telnet negotiation or reconnect automation have executed inside the guest. That is the next runtime qualification layer.

All evidence is written below `build/fs-uae/` and uploaded by the workflow.
