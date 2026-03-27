#!/usr/bin/env zsh
# scripts/start_remote.sh
# Start the main program on remote device in background

set -euo pipefail

HOST=${1:-yqb}

echo "Stopping any existing main process on $HOST..."
ssh "$HOST" 'killall main 2>/dev/null || true'
sleep 1

echo "Starting main program on $HOST in background..."
ssh "$HOST" 'cd /root && nohup ./main > /tmp/main.log 2>&1 &'

echo "Waiting for program to initialize..."
sleep 3

echo "Checking if program is running..."
if ssh "$HOST" 'pgrep -x main > /dev/null'; then
  echo "✓ Program started successfully"
  echo "Log: ssh $HOST 'tail -f /tmp/main.log'"
else
  echo "✗ Failed to start program"
  echo "Check logs: ssh $HOST 'cat /tmp/main.log'"
  exit 1
fi
