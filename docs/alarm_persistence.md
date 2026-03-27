# 闹钟持久化存储说明

## 概述
闹钟模块已实现完整的 JSON 格式持久化存储功能和闹铃提醒。

## 存储位置
- **文件路径**: `/root/data/alarms.json`
- **备份文件**: `/root/data/alarms.json.tmp` (保存时的临时文件)

## 闹铃功能

### 触发机制
- 系统每秒检查一次是否有到期闹钟
- 检查逻辑在主循环中执行（`demo_module.c`）
- 使用 `alarm_check_due()` 函数进行检查

### 闹铃提醒
当闹钟触发时会：
1. **显示全局弹窗**：
   - 半透明黑色背景遮罩
   - 白色圆角通知框
   - 显示铃铛图标、"闹钟"标题和时间
   - 提供"贪睡"和"关闭"两个按钮

2. **播放闹钟音乐**：
   - 使用 `audio_player` 模块播放
   - 音频文件路径：`/root/data/music/闹钟.flac`
   - 使用 mplayer 的 OSS 驱动
   - 点击"关闭"按钮会停止播放

### 弹窗按钮
- **贪睡按钮**（左侧，灰色）：
  - 暂停闹铃音乐
  - 关闭弹窗
  - TODO: 未来可添加延迟再次提醒功能

- **关闭按钮**（右侧，红色）：
  - 完全关闭闹钟提醒
  - 停止播放音乐
  - 关闭弹窗

### 技术实现
```c
// 注册闹钟触发回调
alarm_register_trigger_cb(on_alarm_triggered);

// 主循环中每秒检查
time_t last_alarm_check = 0;
while (1) {
    lv_timer_handler();
    
    time_t now = time(NULL);
    if (now != last_alarm_check) {
        last_alarm_check = now;
        alarm_check_due();
    }
    
    usleep(5000);
}
```

## JSON 数据格式
```json
[
  {
    "id": "a1701234567",
    "label": "起床闹钟",
    "enabled": true,
    "hour": 7,
    "minute": 30,
    "repeat": [0, 1, 1, 1, 1, 1, 0],
    "snooze_minutes": 10,
    "sound": "default",
    "remove_after_trigger": false
  }
]
```

## 字段说明
- `id`: 唯一标识符 (最多36字符)
- `label`: 闹钟标签/名称 (最多63字符)
- `enabled`: 是否启用 (布尔值)
- `hour`: 小时 (0-23)
- `minute`: 分钟 (0-59)
- `repeat`: 重复日期数组，7个元素代表周日到周六 (0=不重复, 1=重复)
- `snooze_minutes`: 贪睡时长（分钟）
- `sound`: 铃声文件路径 (最多127字符)
- `remove_after_trigger`: 触发后是否删除 (布尔值)

## 自动保存时机
系统会在以下操作后自动保存到 JSON 文件：
- ✅ 添加新闹钟 (`alarm_add`)
- ✅ 更新闹钟 (`alarm_update`)
- ✅ 删除闹钟 (`alarm_remove`)
- ✅ 启用/禁用闹钟 (`alarm_enable`)
- ✅ 程序退出时 (`alarm_shutdown`)

## 初始化
在 `demo_module.c` 中已添加初始化代码：
```c
// 确保数据目录存在
system("mkdir -p /root/data");

// 初始化闹钟模块（会自动加载 JSON 文件）
alarm_init("/root/data");
```

## 使用的 JSON 库
- **库**: cJSON (位于 `third_party/cjson/`)
- **特性**: 轻量级、无外部依赖、适合嵌入式系统

## 安全性
- 使用原子写入：先写入 `.tmp` 临时文件，然后重命名
- 使用 `fsync()` 确保数据落盘
- 内存安全：所有字符串都有长度限制

## 测试方法
1. 在 UI 中添加闹钟
2. 检查文件是否生成：
   ```bash
   cat /root/data/alarms.json
   ```
3. 重启程序，验证闹钟是否恢复
