# RexxTelnet

RexxTelnet is a lightweight, ARexx-scriptable Telnet client for classic AmigaOS.

The project is designed around two equally important use cases:

1. an interactive Telnet client for BBS and other character-oriented services;
2. an automation client that can be driven through an ARexx port.

## Target

- AmigaOS 2.04 or newer
- Motorola 68000 minimum target
- TCP/IP through `bsdsocket.library`
- no FPU requirement
- small memory footprint
- Bebbo GCC-compatible build

The initial ARexx port name is:

```
REXXTELNET
```

## Status

- M0 foundation: complete
- M1 TCP session core: implemented
- M1.1 session integration/hardening: implemented

Current core capabilities include Telnet framing, deterministic option negotiation, IAC escaping, portable TCP transport interfaces, an Amiga `bsdsocket.library` backend, and a session pump that preserves parser state across receive boundaries.

## Planned ARexx surface

```rexx
ADDRESS REXXTELNET
'CONNECT bbs.example.org 23'
'WAITFOR "login:" 10'
'SENDLINE "guest"'
'STATUS'
'DISCONNECT'
```

The command surface is intentionally specified before the ARexx transport implementation so scripts can remain stable as implementation work progresses.

## Build

Host-side protocol tests:

```sh
make check
```

The production target remains `-m68000`.

## License

MIT.
