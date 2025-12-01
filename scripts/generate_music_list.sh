#!/usr/bin/env bash
# 生成音乐目录的 list.json 文件
# 格式: [ {"name":"xxx.flac","size":12345,"sha256":"<hex>"}, ... ]
# 仅包含允许的扩展: .mp3 .flac .wav
# 使用: ./generate_music_list.sh /var/www/html/music > /var/www/html/music/list.json.tmp && mv /var/www/html/music/list.json.tmp /var/www/html/music/list.json
# 可放入 cron: */5 * * * * /path/generate_music_list.sh /var/www/html/music > /var/www/html/music/list.json.tmp && mv /var/www/html/music/list.json.tmp /var/www/html/music/list.json

set -euo pipefail
IFS=$'\n\t'

MUSIC_DIR=${1:-}
if [[ -z "$MUSIC_DIR" ]]; then
  echo "Usage: $0 <music_directory>" >&2
  exit 1
fi
if [[ ! -d "$MUSIC_DIR" ]]; then
  echo "Directory not found: $MUSIC_DIR" >&2
  exit 1
fi

allowed_ext='mp3 flac wav'
# 输出 JSON 数组
printf '['
first=1
# 使用 find 保持可扩展性
while IFS= read -r -d '' f; do
  base=$(basename "$f")
  ext="${base##*.}"; ext_lower=$(echo "$ext" | tr 'A-Z' 'a-z')
  if ! grep -qw "$ext_lower" <<< "$allowed_ext"; then
    continue
  fi
  size=$(stat -c %s "$f")
  sha=$(sha256sum "$f" | cut -d' ' -f1)
  # 过滤不安全文件名: 只允许 A-Za-z0-9._-
  if [[ ! "$base" =~ ^[A-Za-z0-9._-]+$ ]]; then
    continue
  fi
  if [[ $first -eq 0 ]]; then
    printf ','
  fi
  first=0
  printf '\n{"name":"%s","size":%s,"sha256":"%s"}' "$base" "$size" "$sha"
# -print0 for null-delimited filenames
done < <(find "$MUSIC_DIR" -maxdepth 1 -type f -print0)
printf '\n]'
