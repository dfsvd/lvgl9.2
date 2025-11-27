#!/bin/sh
# scripts/play_alarm.sh - play a WAV file on device using mplayer (OSS)
set -euo pipefail
if [ $# -lt 1 ]; then echo "Usage: $0 /path/to/sound.wav"; exit 2; fi
file=$1
if command -v mplayer >/dev/null 2>&1; then
  # try OSS output which is available on the target
  mplayer -ao oss "$file"
else
  echo "mplayer not found; try installing or use aplay if available"
  exit 1
fi
