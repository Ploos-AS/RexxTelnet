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
- scrollback
- resizing and NAWS

## M3 — ARexx

- public port `REXXTELNET`
- command parser
- CONNECT / DISCONNECT
- SEND / SENDLINE
- STATUS / GET / SET
- error and result conventions

## M4 — Automation

- receive buffer
- WAITFOR with timeout
- READ / PEEK
- capture/logging
- SENDHEX
- robust disconnect/reconnect semantics

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
