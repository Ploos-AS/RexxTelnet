# RexxTelnet roadmap

## M0 — Foundation

Buildable protocol core, compatibility contract, ARexx API contract and tests.

## M1 — TCP session core

- bsdsocket.library lifecycle
- DNS/IPv4 connection
- connect/disconnect
- socket send/receive
- Telnet negotiation policy
- host-side tests for policy/encoding

## M2 — Interactive terminal

- Amiga console/terminal UI
- keyboard input
- ANSI/VT100 baseline
- resizing and NAWS
- interactive socket/console event loop

## M3 — ARexx and automation

### M3.0 — Port foundation

- public port `REXXTELNET`
- command parser
- RC/result conventions

### M3.1 — Runtime dispatch

- CONNECT / DISCONNECT
- SEND / SENDLINE
- STATUS / GET / SET / QUIT
- RexxMsg replies

### M3.2 — Expect-style receive automation

- receive buffer
- WAITFOR with timeout
- READ / PEEK
- quoted search text

### M3.3 — Capture and binary automation

- CAPTURE / CAPTURE STOP
- SENDHEX
- decoded application-stream observer
- richer Telnet/session properties

## M4 — Hardening and feature completion

- partial-write/send-all handling
- stronger transport/error lifecycle
- reconnect and repeated-negotiation hardening
- TTYPE support
- local-echo behavior tied to Telnet ECHO state
- capture/logging safety controls
- ARexx result-size and binary-data behavior
- final supported command/property contract

## M5 — Qualification

- Bebbo GCC `-m68000` native build
- AmigaOS 2.04 runtime qualification
- AmigaOS 3.x regression
- real BBS Telnet interoperability
- ARexx automation qualification
- reconnect/stress tests

## M6 — Release

- documentation
- example ARexx scripts
- Aminet-ready archive
- checksums
- v1.0.0 release criteria
