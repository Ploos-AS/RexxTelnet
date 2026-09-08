#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="${1:-build/fs-uae/aros-guest}"
SYSTEM_DIR="build/fs-uae/aros-system"
NATIVE="build/fs-uae/native/RexxTelnet"
mkdir -p "$OUT_DIR"

fail() {
  printf 'STATUS=FAIL\nGATE=M5_3A_AROS_GUEST_TCP_RUNTIME\nREASON=%s\n' "$1" | tee "$OUT_DIR/result.txt"
  exit 1
}

[[ -f "$NATIVE" ]] || fail "NATIVE_BINARY_MISSING"

iso="$(bash ci/fs-uae/fetch-aros-system.sh "$SYSTEM_DIR" | tail -n 1)"
root_extract="$OUT_DIR/system-root"
rm -rf "$root_extract"
mkdir -p "$root_extract"
7z x -y -o"$root_extract" "$iso" >/dev/null

startup="$(find "$root_extract" -type f -ipath '*/s/startup-sequence' -print -quit)"
[[ -n "$startup" ]] || fail "STARTUP_SEQUENCE_NOT_FOUND"
aros_root="$(dirname "$(dirname "$startup")")"

rx_host="$(find "$aros_root" -type f -iname 'rx' -print -quit || true)"
rexxmast_host="$(find "$aros_root" -type f -iname 'rexxmast' -print -quit || true)"
rexxlib_host="$(find "$aros_root" -type f -iname 'rexxsyslib.library' -print -quit || true)"
{
  echo "RX=${rx_host:-MISSING}"
  echo "REXXMAST=${rexxmast_host:-MISSING}"
  echo "REXXSYSLIB=${rexxlib_host:-MISSING}"
  echo "NOTE=ARexx capability is observational only in M5.3a"
} > "$OUT_DIR/arexx-capabilities.txt"

cp "$NATIVE" "$aros_root/RexxTelnet"
cp "$startup" "$startup.rexxtelnet-original"

# Launch only after the normal AROS Startup-Sequence has initialized assigns,
# handlers, preferences and user startup.  The previous harness launched at
# the first line of Startup-Sequence, which was too early to prove a normal
# application runtime environment.
python3 - "$startup.rexxtelnet-original" "$startup" <<'PY'
from pathlib import Path
import sys

src = Path(sys.argv[1]).read_text()
marker = 'If EXISTS "WANDERER:Wanderer"'
if marker not in src:
    raise SystemExit('WANDERER_START_MARKER_NOT_FOUND')
probe = '''SYS:C/Echo "M5_3A_GUEST_STARTED=1" >SYS:m5-3a-started.txt
Run <NIL: >SYS:m5-3a-rexxtelnet-output.txt SYS:RexxTelnet 127.0.0.1 2323
SYS:C/Wait 8
SYS:C/Status >SYS:m5-3a-status.txt
SYS:C/Echo "M5_3A_POST_LAUNCH=1" >SYS:m5-3a-post-launch.txt

'''
Path(sys.argv[2]).write_text(src.replace(marker, probe + marker, 1))
PY

rm -f \
  "$aros_root/m5-3a-started.txt" \
  "$aros_root/m5-3a-post-launch.txt" \
  "$aros_root/m5-3a-rexxtelnet-output.txt" \
  "$aros_root/m5-3a-status.txt"

cat > "$OUT_DIR/telnet-server.py" <<'PY'
import socket
import pathlib

out = pathlib.Path('build/fs-uae/aros-guest/telnet-server.txt')
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('127.0.0.1', 2323))
s.listen(1)
s.settimeout(40)
try:
    conn, addr = s.accept()
    out.write_text('ACCEPTED=1\nPEER=%s:%s\n' % addr)
    conn.sendall(b'RexxTelnet M5.3a ready\r\n')
    conn.settimeout(10)
    try:
        while conn.recv(1024):
            pass
    except (socket.timeout, ConnectionResetError):
        pass
    conn.close()
except Exception as exc:
    out.write_text('ACCEPTED=0\nERROR=%r\n' % (exc,))
finally:
    s.close()
PY
python3 "$OUT_DIR/telnet-server.py" &
server_pid=$!
trap 'kill "$server_pid" 2>/dev/null || true' EXIT
sleep 1

config="$OUT_DIR/aros-guest.fs-uae"
sed "s|@AROS_ROOT@|$PWD/$aros_root|" ci/fs-uae/aros-guest.fs-uae > "$config"
fs-uae --version > "$OUT_DIR/fs-uae-version.txt" 2>&1 || true

set +e
timeout 45s xvfb-run -a fs-uae "$config" > "$OUT_DIR/fs-uae.log" 2>&1
rc=$?
set -e
wait "$server_pid" 2>/dev/null || true
trap - EXIT

started="$aros_root/m5-3a-started.txt"
post_launch="$aros_root/m5-3a-post-launch.txt"
run_output="$aros_root/m5-3a-rexxtelnet-output.txt"
status_output="$aros_root/m5-3a-status.txt"
server_out="$OUT_DIR/telnet-server.txt"
status=FAIL
observation=guest_tcp_evidence_incomplete

if [[ -f "$started" && -f "$server_out" ]] \
   && grep -q 'ACCEPTED=1' "$server_out"; then
  status=PASS
  observation=native_rexxtelnet_launched_and_connected_via_bsdsocket
fi

{
  echo "STATUS=$status"
  echo "GATE=M5_3A_AROS_GUEST_TCP_RUNTIME"
  echo "MODEL=A1200"
  echo "KICKSTART=internal"
  echo "BSD_SOCKET_EMULATION=1"
  echo "FS_UAE_EXIT=$rc"
  echo "OBSERVATION=$observation"
  echo "AREXX_QUALIFICATION=M5.3b_LOCAL_CLASSIC_AMIGAOS"
  echo "AROS_REXXMAST=${rexxmast_host:-MISSING}"
  echo "AROS_REXXSYSLIB=${rexxlib_host:-MISSING}"
  echo "GUEST_STARTED=$([[ -f "$started" ]] && echo 1 || echo 0)"
  echo "POST_LAUNCH=$([[ -f "$post_launch" ]] && echo 1 || echo 0)"
  if [[ -f "$server_out" ]]; then tr -d '\r' < "$server_out" | sed 's/^/SERVER_/' ; fi
  if [[ -f "$run_output" ]]; then tr -d '\r' < "$run_output" | sed 's/^/REXXTELNET_OUTPUT_/' ; fi
  if [[ -f "$status_output" ]]; then tr -d '\r' < "$status_output" | sed 's/^/GUEST_STATUS_/' ; fi
} | tee "$OUT_DIR/result.txt"

[[ "$status" == PASS ]]
