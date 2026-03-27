#!/usr/bin/env zsh
# scripts/capture_now.sh
# Ensure program is running and capture framebuffer

set -euo pipefail

HOST=${1:-yqb}
OUT=${2:-}

# Check if program is running
if ! ssh "$HOST" 'pgrep -x main > /dev/null 2>&1'; then
  echo "Program not running, starting it first..."
  ./scripts/start_remote.sh "$HOST"
fi

# Capture
if [[ -n "$OUT" ]]; then
  ./scripts/fb_capture.sh "$HOST" -o "$OUT"
else
  ./scripts/fb_capture.sh "$HOST"
fi
