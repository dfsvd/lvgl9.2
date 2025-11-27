#!/usr/bin/env zsh
# scripts/fb_capture.sh
# Capture framebuffer from a remote board and convert to PNG using ffmpeg.
# Usage:
#   ./scripts/fb_capture.sh [user@host] [-o out.png] [-H visible_height] [-p pixfmt] [-f] [--no-clean]
# Defaults: host="yqb", out.png="fb_top.png", pixfmt="bgra".

set -euo pipefail

HOST=${1:-yqb}
OUT=""
PIXFMT="bgra"
VISIBLE_HEIGHT=""
NO_CLEAN=0
VFLIP=0

usage(){
  cat <<EOF
Usage: $0 [user@host] [-o out.png] [-H visible_height] [-p pixel_format] [-f] [--no-clean]
  -o FILE    Output PNG filename (default fb_top.png)
  -H HEIGHT  Visible framebuffer height to crop to (defaults to geometry from fbset if available)
  -p PIXFMT  Pixel format for ffmpeg (default bgra)
  -f         Flip vertically after crop (useful if image is upside-down)
  --no-clean Don't remove remote /tmp/fb.raw after copy
EOF
  exit 1
}

# parse options
shift_count=0
if [[ "$#" -gt 0 ]] && [[ ! "$1" =~ ^- ]]; then
  HOST="$1"
  shift
fi

while [[ $# -gt 0 ]]; do
  case "$1" in
    -o) OUT="$2"; shift 2;;
    -H) VISIBLE_HEIGHT="$2"; shift 2;;
    -p) PIXFMT="$2"; shift 2;;
    -f) VFLIP=1; shift;;
    --no-clean) NO_CLEAN=1; shift;;
    -h|--help) usage;;
    *) echo "Unknown arg: $1"; usage;;
  esac
done

# If OUT not provided, generate default: {board}_{HH_MM}.png
if [[ -z "$OUT" ]]; then
  board=${HOST##*@}
  ts=$(date +%H_%M)
  OUT="${board}_${ts}.png"
fi

for cmd in ssh scp ffmpeg; do
  if ! command -v $cmd &>/dev/null; then
    echo "Required tool '$cmd' not found on host. Install it and retry." >&2
    exit 2
  fi
done

REMOTE_RAW=/tmp/fb.raw

echo "Querying remote framebuffer info on $HOST..."
VIRTUAL_SIZE=$(ssh "$HOST" 'cat /sys/class/graphics/fb0/virtual_size 2>/dev/null || true')
BPP=$(ssh "$HOST" 'cat /sys/class/graphics/fb0/bits_per_pixel 2>/dev/null || true')

if [[ -z "$VIRTUAL_SIZE" ]]; then
  echo "Failed to read /sys/class/graphics/fb0/virtual_size on remote host." >&2
  exit 3
fi

virtual_w=${${(s/,/)VIRTUAL_SIZE}[1]}
virtual_h=${${(s/,/)VIRTUAL_SIZE}[2]}

echo "Remote virtual_size=${virtual_w}x${virtual_h}, bits_per_pixel=${BPP:-unknown}"

# Try to get visible geometry via fbset
FBSET_OUT=$(ssh "$HOST" 'fbset -s 2>/dev/null || true')
if [[ -n "$FBSET_OUT" ]]; then
  # parse geometry line: geometry W H ...
  geom_line=$(echo "$FBSET_OUT" | awk '/geometry/ {print $0; exit}') || true
  if [[ -n "$geom_line" ]]; then
    visible_w=$(echo "$geom_line" | awk '{print $2}')
    visible_h=$(echo "$geom_line" | awk '{print $3}')
    echo "Parsed visible geometry from fbset: ${visible_w}x${visible_h}"
  fi
fi

if [[ -z "$visible_w" || -z "$visible_h" ]]; then
  if [[ -n "$VISIBLE_HEIGHT" ]]; then
    visible_h="$VISIBLE_HEIGHT"
    visible_w="$virtual_w"
    echo "Using user-supplied visible height: ${visible_w}x${visible_h}"
  else
    # fallback: assume visible height equals first geometry height if virtual is tall
    visible_w="$virtual_w"
    visible_h="$virtual_h"
    echo "fbset not available; defaulting visible size to virtual size: ${visible_w}x${visible_h}"
  fi
fi

echo "Triggering remote framebuffer dump to ${REMOTE_RAW}..."
ssh "$HOST" "dd if=/dev/fb0 of=${REMOTE_RAW} bs=4096 conv=sync" || {
  echo "Remote dd failed" >&2; exit 4
}

echo "Copying ${REMOTE_RAW} to local directory..."
scp "$HOST:${REMOTE_RAW}" ./fb.raw || { echo "scp failed" >&2; exit 5 }

echo "Converting raw framebuffer to PNG (${virtual_w}x${virtual_h} -> crop ${visible_w}x${visible_h})"
VFILT="crop=${visible_w}:${visible_h}:0:0"
if [[ $VFLIP -eq 1 ]]; then
  VFILT+",vflip"
fi

ffmpeg -f rawvideo -pixel_format ${PIXFMT} -video_size ${virtual_w}x${virtual_h} -i fb.raw -vf "${VFILT}" -frames:v 1 "${OUT}"

echo "Generated ${OUT}"

# remove local intermediate raw file
if [[ -f ./fb.raw ]]; then
  rm -f ./fb.raw
  echo "Removed local intermediate fb.raw"
fi

if [[ $NO_CLEAN -eq 0 ]]; then
  echo "Removing remote temporary ${REMOTE_RAW}..."
  ssh "$HOST" "rm -f ${REMOTE_RAW}" || true
fi

echo "Done."
