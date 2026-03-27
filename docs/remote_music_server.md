# 远程音乐服务器部署与协议

## 目标
提供设备可访问的音乐文件列表与文件本体, 让客户端通过 `list.json` 获取所有可下载的音频并选择下载。

## 协议文件: list.json
- 路径示例: `https://your-domain/music/list.json`
- 内容: JSON 数组, 每个元素包含
  - `name`: 文件名 (仅字母数字及 `._-`)
  - `size`: 文件字节大小
  - `sha256`: 文件 SHA-256 校验 (可用于完整性验证)

示例:
```json
[
  {"name": "track1.flac", "size": 12345678, "sha256": "abcdef123..."},
  {"name": "song2.mp3", "size": 456789, "sha256": "7890abcde..."}
]
```

## 生成脚本
仓库中 `scripts/generate_music_list.sh` 可在服务器运行, 传入音乐目录路径:
```bash
./generate_music_list.sh /var/www/html/music > /var/www/html/music/list.json.tmp \
  && mv /var/www/html/music/list.json.tmp /var/www/html/music/list.json
```
使用 `mv` 原子替换避免客户端读到部分写入。

### 定时更新 (cron)
```cron
*/5 * * * * /path/to/generate_music_list.sh /var/www/html/music > /var/www/html/music/list.json.tmp && mv /var/www/html/music/list.json.tmp /var/www/html/music/list.json
```

## Web 服务器 (Nginx 示例)
```nginx
location /music/ {
    alias /var/www/html/music/;
    autoindex off; # 关闭目录索引以只允许明确文件访问
    add_header Access-Control-Allow-Origin *;
    add_header Cache-Control "no-cache";
}
```
确保 `list.json` 与音频文件都在该目录下。

## 安全与过滤
- 文件名白名单: 只允许 `[A-Za-z0-9._-]`, 脚本会跳过非法名称。
- 不允许子目录递归 (maxdepth 1)。
- 可以添加 Token 保护: 将 URL 变为 `list.json?token=<X>` 并在 Nginx 或后端校验。

## 可选扩展
1. 增加字段 `mtime` (最后修改 UNIX 秒时间) 用于客户端排序或增量更新。
2. 增加字段 `duration` 预解析音频时长 (需 ffprobe 或 mediainfo), 客户端可显示预计长度。
3. 支持分页: 若文件很多, 输出 `{"total":N,"files":[...]}` 结构。
4. 断点续传: 客户端下载用 `curl -C -` 实现中断重试。
5. 压缩批量: 提供打包 zip 下载多个文件。

## 客户端使用概要
1. 访问 `list.json` -> 解析数组.
2. 对每个 `name` 构造下载 URL: `REMOTE_MUSIC_BASE_URL + '/' + name`.
3. 下载保存到设备: `/root/data/music/<name>`.
4. 可选: 下载后计算 SHA-256 与 `sha256` 对比; 不匹配则提示失败并删除文件。
5. 更新本地播放列表并通知 UI。

## 故障排查
- `list.json` 空: 检查脚本权限与 cron 输出是否被重定向。
- 某文件缺失: 确认扩展名在允许列表或文件名不含非法字符。
- 客户端解析失败: 使用 `jq . list.json` 验证 JSON 语法。
- 传输慢: 考虑开启 HTTP/2 或 CDN 缓存静态音频并保留 no-cache 仅对 `list.json`。

## 后续验证
下载几个测试文件后检查客户端是否正确刷新与播放。若需哈希校验, 在客户端添加对 `sha256` 的比对逻辑再更新 UI。
