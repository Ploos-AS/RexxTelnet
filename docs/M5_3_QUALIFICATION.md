# M5.3 qualification

M5.3 is intentionally split because the public AROS m68k system image used in CI does not provide the complete classic ARexx runtime required by RexxTelnet.

## M5.3a — automated AROS guest TCP runtime

Run in GitHub Actions with FS-UAE and the legal AROS replacement ROM/system image.

PASS requires:

1. Host regression tests pass.
2. FS-UAE boots with the internal AROS replacement ROM.
3. AROS boot media remains viable.
4. RexxTelnet builds with the pinned Bebbo GCC image for `-m68000 -noixemul`.
5. The native RexxTelnet executable is copied into the AROS guest filesystem and launched by the guest startup sequence.
6. RexxTelnet reaches a controlled TCP listener through FS-UAE `bsdsocket_library = 1` emulation.

The AROS image is also inspected for `RX`, `RexxMast` and `rexxsyslib.library`, but their presence is observational only for M5.3a. Missing classic ARexx components do not fail M5.3a.

M5.3a does not claim ARexx qualification.

## M5.3b — local classic AmigaOS ARexx runtime

Run locally under a licensed/classic AmigaOS FS-UAE environment with RexxMast and `rexxsyslib.library` available.

Repository helpers:

- `ci/local/m5_3b_telnet_server.py` — controlled host-side TCP endpoint, default port 2323.
- `ci/local/m5_3b_arexx.rexx` — ARexx smoke/automation script for the public `REXXTELNET` port.

Suggested environment:

- A1200-class FS-UAE machine.
- AmigaOS 2.04+; AmigaOS 3.x is suitable for the first qualification run.
- `bsdsocket_library = 1` or a working compatible TCP/IP stack.
- RexxMast running before RexxTelnet.
- Native `build/fs-uae/native/RexxTelnet` binary from the pinned Bebbo build, or an equivalent locally built `-m68000` binary.

### Procedure

1. Start the controlled server on the host:

   `python3 ci/local/m5_3b_telnet_server.py 2323`

2. Start the AmigaOS guest with networking enabled.
3. Ensure RexxMast is running.
4. Launch RexxTelnet against `127.0.0.1 2323` when FS-UAE bsdsocket emulation maps guest sockets to the host. If the local networking setup uses another reachable address, use that address instead.
5. Execute `ci/local/m5_3b_arexx.rexx` inside the guest using `RX`, capturing its output.

### Required evidence

The ARexx output must show all of the following:

- `M5_3B_STATUS=CONNECTED`
- `M5_3B_RXCAPACITY=4096`
- `M5_3B_WAITFOR_LOGIN_RC=0`
- `M5_3B_SENDLINE_RC=0`
- `M5_3B_WAITFOR_WELCOME_RC=0`
- `M5_3B_RXDROPPED=0` for the controlled short exchange
- `M5_3B_QUIT_RC=0`
- `M5_3B_DONE=1`

The host server output must show:

- `M5_3B_ACCEPTED=...`
- `M5_3B_SENDLINE_OK=1`

Record the AmigaOS version, machine profile, FS-UAE version, RexxTelnet binary SHA-256, exact launch command, ARexx output and server output.

## Qualification status

- M5.3a: automated CI gate.
- M5.3b: local qualification; must not be marked PASS until real classic AmigaOS/ARexx evidence has been captured.
