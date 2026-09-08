#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="${1:-build/fs-uae/aros-guest}"
SYSTEM_DIR="build/fs-uae/aros-system"
NATIVE="build/fs-uae/native/RexxTelnet"
mkdir -p "$OUT_DIR"

fail() {
  printf 'STATUS=FAIL\nGATE=M5_3_AROS_GUEST_RUNTIME\nREASON=%s\n' "$1" | tee "$OUT_DIR/result.txt"
  exit 1
}

[[ -f "$NATIVE" ]] || fail "NATIVE_BINARY_MISSING"

iso="$(ci/fs-uae/fetch-aros-system.sh "$SYSTEM_DIR" | tail -n 1)"
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
} > "$OUT_DIR/arexx-capabilities.txt"

[[ -n "$rx_host" ]] || fail "AROS_RX_NOT_FOUND"
[[ -n "$rexxmast_host" ]] || fail "AROS_REXXMAST_NOT_FOUND"
[[ -n "$rexxlib_host" ]] || fail "AROS_REXXSYSLIB_NOT_FOUND"

amiga_path() {
  local rel="${1#"$aros_root"/}"
  printf 'SYS:%s' "$rel"
}
rx_amiga="$(amiga_path "$rx_host")"
rexxmast_amiga="$(amiga_path "$rexxmast_host")"

cp "$NATIVE" "$aros_root/RexxTelnet"
cp "$startup" "$startup.rexxtelnet-original"

cat > "$aros_root/m5_3.rexx" <<'EOF'
ADDRESS REXXTELNET
'STATUS'
SAY 'STATUS=' RESULT
'GET RXCAPACITY'
SAY 'RXCAPACITY=' RESULT
'GET RXBYTES'
SAY 'RXBYTES=' RESULT
'QUIT'
SAY 'QUIT_RC=' RC
EOF

cat > "$startup" <<EOF
SYS:C/Echo "M5_3_GUEST_STARTED=1" >SYS:m5-3-started.txt
Run >NIL: $rexxmast_amiga
SYS:C/Wait 2
Run >NIL: SYS:RexxTelnet 127.0.0.1 2323
SYS:C/Wait 3
$rx_amiga SYS:m5_3.rexx >SYS:m5-3-arexx.txt
SYS:C/Wait 2
SYS:C/Execute SYS:S/Startup-Sequence.rexxtelnet-original
EOF

rm -f "$aros_root/m5-3-started.txt" "$aros_root/m5-3-arexx.txt"

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
    conn.sendall(b'RexxTelnet M5.3 ready\r\n')
    conn.settimeout(20)
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
timeout 55s xvfb-run -a fs-uae "$config" > "$OUT_DIR/fs-uae.log" 2>&1
rc=$?
set -e
wait "$server_pid" 2>/dev/null || true
trap - EXIT

started="$aros_root/m5-3-started.txt"
arexx_out="$aros_root/m5-3-arexx.txt"
server_out="$OUT_DIR/telnet-server.txt"
status=FAIL
observation=guest_runtime_evidence_incomplete

if [[ -f "$started" && -f "$arexx_out" && -f "$server_out" ]] \
   && grep -q 'ACCEPTED=1' "$server_out" \
   && grep -q 'STATUS=CONNECTED' "$arexx_out" \
   && grep -q 'RXCAPACITY=4096' "$arexx_out"; then
  status=PASS
  observation=guest_connected_and_arexx_port_answered
fi

{
  echo "STATUS=$status"
  echo "GATE=M5_3_AROS_GUEST_RUNTIME"
  echo "MODEL=A1200"
  echo "KICKSTART=internal"
  echo "BSD_SOCKET_EMULATION=1"
  echo "FS_UAE_EXIT=$rc"
  echo "OBSERVATION=$observation"
  if [[ -f "$server_out" ]]; then tr -d '\r' < "$server_out" | sed 's/^/SERVER_/' ; fi
  if [[ -f "$arexx_out" ]]; then tr -d '\r' < "$arexx_out" | sed 's/^/AREXX_/' ; fi
} | tee "$OUT_DIR/result.txt"

[[ "$status" == PASS ]]
