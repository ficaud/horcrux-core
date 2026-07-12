#!/usr/bin/env bash
# ============================================================================
# host-flash.sh – Flash ESP32-S3 from macOS host
# ============================================================================
# Usage:
#
#   ./tools/host-flash.sh                   # use local build
#   ./tools/host-flash.sh path/to/firmware.bin  # use a custom binary
#
# The script uses esptool directly on the Mac's serial port.
# The default binary path is <project>/build/zephyr/zephyr.bin (mounted
# from the dev container).
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# ── Serial port detection ──
PORT=$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)

if [ -z "$PORT" ]; then
  echo "❌ No ESP32 port found (looked for /dev/cu.usbmodem*)"
  echo "   Plug in the board and check with: ls /dev/cu.usbmodem*"
  exit 1
fi

echo "🔌 Detected port: $PORT"

# ── Ensure esptool is available ──
if ! python3 -c "import esptool" 2>/dev/null; then
  echo "📦 Installing esptool…"
  pip3 install esptool
fi

# ── Binary to flash ──
BIN="${1:-$SCRIPT_DIR/build/zephyr/zephyr.bin}"

if [ ! -f "$BIN" ]; then
  echo "❌ Binary not found: $BIN"
  echo "   Build first, or provide a path: $0 path/to/firmware.bin"
  exit 1
fi

echo "⚡ Flashing $BIN …"
echo "   Hold BOOT, tap RESET, release BOOT."
esptool --chip esp32s3 --port "$PORT" --baud 921600 \
  --before default-reset --after hard-reset \
  write-flash 0x0 "$BIN"
