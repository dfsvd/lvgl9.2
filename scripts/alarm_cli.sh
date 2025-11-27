#!/usr/bin/env bash
# scripts/alarm_cli.sh - simple helper to add/list/remove alarms by editing data/alarms.json
# Usage: alarm_cli.sh add|list|rm <args>
set -euo pipefail
WD="$(cd "$(dirname "$0")/.." && pwd)"
DATA="$WD/data"
FILE="$DATA/alarms.json"
mkdir -p "$DATA"

cmd=${1:-}
case "$cmd" in
  list)
    if [ -f "$FILE" ]; then cat "$FILE" | jq '.' || cat "$FILE" ; else echo "[]"; fi
    ;;
  add)
    # add <id> <hour> <minute> <label>
    if [ $# -lt 5 ]; then echo "Usage: $0 add <id> <hour> <minute> <label>"; exit 2; fi
    id=$2; hour=$3; minute=$4; shift 4; label="$*"
    tmp=$(mktemp)
    if [ -f "$FILE" ]; then jq ". + [{\"id\": \"$id\", \"label\": \"$label\", \"enabled\": true, \"hour\": $hour, \"minute\": $minute, \"repeat\": [0,0,0,0,0,0,0], \"snooze_minutes\": 10, \"sound\": \"/tmp/alarm.wav\", \"remove_after_trigger\": false}]" "$FILE" > "$tmp"; else echo "[{\"id\": \"$id\", \"label\": \"$label\", \"enabled\": true, \"hour\": $hour, \"minute\": $minute, \"repeat\": [0,0,0,0,0,0,0], \"snooze_minutes\": 10, \"sound\": \"/tmp/alarm.wav\", \"remove_after_trigger\": false}]" > "$tmp"; fi
    mv "$tmp" "$FILE"
    echo "Added $id" ;;
  rm)
    if [ $# -ne 2 ]; then echo "Usage: $0 rm <id>"; exit 2; fi
    id=$2
    if [ ! -f "$FILE" ]; then echo "no file"; exit 1; fi
    tmp=$(mktemp)
    jq "map(select(.id != \"$id\"))" "$FILE" > "$tmp"
    mv "$tmp" "$FILE"
    echo "Removed $id" ;;
  *)
    echo "Usage: $0 {list|add|rm} ..."; exit 2;;
esac
