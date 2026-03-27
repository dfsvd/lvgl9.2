#!/usr/bin/env zsh
# scripts/stop_remote.sh
# Stop the main program on remote device

set -euo pipefail

HOST=${1:-yqb}

echo "Stopping main program on $HOST..."
ssh "$HOST" 'killall main 2>/dev/null || killall -9 main 2>/dev/null || true'

echo "Stopping any mplayer processes..."
ssh "$HOST" 'killall mplayer 2>/dev/null || killall -9 mplayer 2>/dev/null || true'

sleep 1

if ssh "$HOST" 'pgrep -x main > /dev/null'; then
  echo "✗ Failed to stop program"
  exit 1
else
  echo "✓ Program stopped"
fi
