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

# M5.3a uses a minimal CI-only Startup-Sequence. Each initialization step is
# bracketed by a persistent SYS: marker so a timed-out guest identifies the
# exact command that did not return.
cat > "$startup" <<'AROS_STARTUP'
FailAt 21

SYS:C/Echo "M5_3A_GUEST_STARTED=1" >SYS:m5-3a-started.txt

SYS:C/Echo "BEFORE_MKDIR_CLIPBOARDS=1" >SYS:m5-3a-before-mkdir-clipboards.txt
If NOT EXISTS "RAM:Clipboards"
    SYS:C/MakeDir "RAM:Clipboards"
EndIf
SYS:C/Echo "AFTER_MKDIR_CLIPBOARDS=1" >SYS:m5-3a-after-mkdir-clipboards.txt

SYS:C/Echo "BEFORE_MKDIR_T=1" >SYS:m5-3a-before-mkdir-t.txt
If NOT EXISTS "RAM:T"
    SYS:C/MakeDir "RAM:T"
EndIf
SYS:C/Echo "AFTER_MKDIR_T=1" >SYS:m5-3a-after-mkdir-t.txt

SYS:C/Echo "BEFORE_MKDIR_ENV=1" >SYS:m5-3a-before-mkdir-env.txt
If NOT EXISTS "RAM:ENV"
    SYS:C/MakeDir "RAM:ENV"
EndIf
SYS:C/Echo "AFTER_MKDIR_ENV=1" >SYS:m5-3a-after-mkdir-env.txt

SYS:C/Echo "BEFORE_ASSIGN_T=1" >SYS:m5-3a-before-assign-t.txt
SYS:C/Assign "T:" "RAM:T"
SYS:C/Echo "AFTER_ASSIGN_T=1" >SYS:m5-3a-after-assign-t.txt

SYS:C/Echo "BEFORE_ASSIGN_CLIPS=1" >SYS:m5-3a-before-assign-clips.txt
SYS:C/Assign "CLIPS:" "RAM:Clipboards"
SYS:C/Echo "AFTER_ASSIGN_CLIPS=1" >SYS:m5-3a-after-assign-clips.txt

SYS:C/Echo "BEFORE_ASSIGN_ENV=1" >SYS:m5-3a-before-assign-env.txt
SYS:C/Assign "ENV:" "RAM:ENV"
SYS:C/Echo "AFTER_ASSIGN_ENV=1" >SYS:m5-3a-after-assign-env.txt

SYS:C/Echo "BEFORE_ASSIGN_LIBS=1" >SYS:m5-3a-before-assign-libs.txt
SYS:C/Assign "LIBS:" "SYS:Libs"
SYS:C/Echo "AFTER_ASSIGN_LIBS=1" >SYS:m5-3a-after-assign-libs.txt

SYS:C/Echo "BEFORE_ASSIGN_CLASSES=1" >SYS:m5-3a-before-assign-classes.txt
SYS:C/Assign "LIBS:" "SYS:Classes" ADD
SYS:C/Echo "AFTER_ASSIGN_CLASSES=1" >SYS:m5-3a-after-assign-classes.txt

SYS:C/Echo "BEFORE_SETPATCH=1" >SYS:m5-3a-before-setpatch.txt
If EXISTS "C:SetPatch"
    C:SetPatch QUIET
EndIf
SYS:C/Echo "AFTER_SETPATCH=1" >SYS:m5-3a-after-setpatch.txt

SYS:C/Echo "BEFORE_AUTOMOUNT=1" >SYS:m5-3a-before-automount.txt
SYS:C/Automount >NIL:
SYS:C/Echo "AFTER_AUTOMOUNT=1" >SYS:m5-3a-after-automount.txt

SYS:C/Echo "BEFORE_MOUNT=1" >SYS:m5-3a-before-mount.txt
SYS:C/Mount >NIL: "DEVS:DOSDrivers/~((.#?)|(#?.info)|(#?.dbg))"
SYS:C/Echo "AFTER_MOUNT=1" >SYS:m5-3a-after-mount.txt

SYS:C/Echo "BEFORE_PIPE=1" >SYS:m5-3a-before-pipe.txt
SYS:C/Dir >NIL: "PIPE:"
SYS:C/Echo "AFTER_PIPE=1" >SYS:m5-3a-after-pipe.txt

SYS:C/Echo "BEFORE_PATH=1" >SYS:m5-3a-before-path.txt
SYS:C/Path "C:" "SYS:System" "S:" "SYS:Prefs" "SYS:Tools" "SYS:Utilities" QUIET
SYS:C/Echo "AFTER_PATH=1" >SYS:m5-3a-after-path.txt

SYS:C/Echo "M5_3A_RUNTIME_READY=1" >SYS:m5-3a-runtime-ready.txt
Run <NIL: >SYS:m5-3a-rexxtelnet-output.txt SYS:RexxTelnet 127.0.0.1 2323
SYS:C/Echo "M5_3A_RUN_RETURNED=1" >SYS:m5-3a-run-returned.txt
SYS:C/Wait 8
SYS:C/Status >SYS:m5-3a-status.txt
SYS:C/Echo "M5_3A_POST_LAUNCH=1" >SYS:m5-3a-post-launch.txt
SYS:C/Wait 60
AROS_STARTUP

rm -f \
  "$aros_root"/m5-3a-*.txt

cat > "$OUT_DIR/telnet-server.py" <<'PY'
import socket
import pathlib

out = pathlib.Path('build/fs-uae/aros-guest/telnet-server.txt')
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('127.0.0.1', 2323))
s.listen(1)
s.settimeout(80)
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
timeout 90s xvfb-run -a fs-uae "$config" > "$OUT_DIR/fs-uae.log" 2>&1
rc=$?
set -e
wait "$server_pid" 2>/dev/null || true
trap - EXIT

started="$aros_root/m5-3a-started.txt"
runtime_ready="$aros_root/m5-3a-runtime-ready.txt"
run_returned="$aros_root/m5-3a-run-returned.txt"
post_launch="$aros_root/m5-3a-post-launch.txt"
run_output="$aros_root/m5-3a-rexxtelnet-output.txt"
status_output="$aros_root/m5-3a-status.txt"
server_out="$OUT_DIR/telnet-server.txt"
status=FAIL
observation=guest_tcp_evidence_incomplete

if [[ -f "$started" && -f "$runtime_ready" && -f "$server_out" ]] \
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
  echo "STARTUP_MODE=minimal_ci_instrumented"
  echo "FS_UAE_EXIT=$rc"
  echo "OBSERVATION=$observation"
  echo "AREXX_QUALIFICATION=M5.3b_LOCAL_CLASSIC_AMIGAOS"
  echo "AROS_REXXMAST=${rexxmast_host:-MISSING}"
  echo "AROS_REXXSYSLIB=${rexxlib_host:-MISSING}"
  echo "GUEST_STARTED=$([[ -f "$started" ]] && echo 1 || echo 0)"
  echo "RUNTIME_READY=$([[ -f "$runtime_ready" ]] && echo 1 || echo 0)"
  echo "RUN_RETURNED=$([[ -f "$run_returned" ]] && echo 1 || echo 0)"
  echo "POST_LAUNCH=$([[ -f "$post_launch" ]] && echo 1 || echo 0)"
  for marker in "$aros_root"/m5-3a-before-*.txt "$aros_root"/m5-3a-after-*.txt; do
    if [[ -f "$marker" ]]; then
      basename "$marker" .txt | tr '[:lower:]-' '[:upper:]_' | sed 's/^/INIT_/' | sed 's/$/=1/'
    fi
  done
  if [[ -f "$server_out" ]]; then tr -d '\r' < "$server_out" | sed 's/^/SERVER_/' ; fi
  if [[ -f "$run_output" ]]; then tr -d '\r' < "$run_output" | sed 's/^/REXXTELNET_OUTPUT_/' ; fi
  if [[ -f "$status_output" ]]; then tr -d '\r' < "$status_output" | sed 's/^/GUEST_STATUS_/' ; fi
} | tee "$OUT_DIR/result.txt"

[[ "$status" == PASS ]]
