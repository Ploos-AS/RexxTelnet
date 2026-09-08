#!/usr/bin/env bash
set -euo pipefail

AROS_INDEX_URL="https://aros.sourceforge.io/cgi-bin/files?lang=en&type=nightly2"
AROS_TARGET="amiga-m68k-boot-floppy"
OUT_DIR="${1:-build/fs-uae/aros-boot}"

mkdir -p "$OUT_DIR"
index_html="$OUT_DIR/aros-nightly-index.html"

curl --fail --location --retry 3 --retry-delay 2 "$AROS_INDEX_URL" -o "$index_html"

AROS_URL="$(grep -oE 'href="[^"]*amiga-m68k-boot-floppy[^"]*"' "$index_html" | head -n 1 | sed -e 's/^href="//' -e 's/"$//' -e 's/&amp;/\&/g')"
[[ -n "$AROS_URL" ]] || { echo "ERROR: could not resolve $AROS_TARGET" >&2; exit 1; }

case "$AROS_URL" in
  http://*|https://*) ;;
  //*) AROS_URL="https:${AROS_URL}" ;;
  /*) AROS_URL="https://aros.sourceforge.io${AROS_URL}" ;;
  *) AROS_URL="https://aros.sourceforge.io/${AROS_URL}" ;;
esac

url_path="${AROS_URL%%\?*}"
if [[ "$url_path" == */download ]]; then
  AROS_ARCHIVE="$(basename "$(dirname "$url_path")")"
else
  AROS_ARCHIVE="$(basename "$url_path")"
fi
[[ "$AROS_ARCHIVE" == *"$AROS_TARGET"* ]] || { echo "ERROR: unexpected AROS archive: $AROS_URL" >&2; exit 1; }

archive="$OUT_DIR/$AROS_ARCHIVE"
curl --fail --location --retry 3 --retry-delay 2 "$AROS_URL" -o "$archive"
sha256sum "$archive" | tee "$OUT_DIR/archive.sha256"

rm -rf "$OUT_DIR/extracted"
mkdir -p "$OUT_DIR/extracted"
case "$AROS_ARCHIVE" in
  *.lha|*.LHA) lha xw="$OUT_DIR/extracted" "$archive" >/dev/null ;;
  *.zip|*.ZIP) unzip -q "$archive" -d "$OUT_DIR/extracted" ;;
  *) echo "ERROR: unsupported archive format: $AROS_ARCHIVE" >&2; exit 1 ;;
esac

adf="$(find "$OUT_DIR/extracted" -type f \( -iname '*.adf' -o -iname '*.ADF' \) | head -n 1)"
[[ -n "$adf" ]] || { echo "ERROR: no ADF found" >&2; exit 1; }
cp "$adf" "$OUT_DIR/bootdisk.adf"
printf 'AROS_INDEX_URL=%s\nAROS_TARGET=%s\nAROS_ARCHIVE=%s\nAROS_URL=%s\nADF=%s\n' "$AROS_INDEX_URL" "$AROS_TARGET" "$AROS_ARCHIVE" "$AROS_URL" "$OUT_DIR/bootdisk.adf" > "$OUT_DIR/source.txt"
echo "$OUT_DIR/bootdisk.adf"
