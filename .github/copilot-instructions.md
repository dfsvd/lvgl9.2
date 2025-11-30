<!-- .github/copilot-instructions.md -->
# LV Port (lv_port_linux-9.2) — Copilot 指南

目的：帮助 AI 编码助手快速在此仓库中进行修改、调试与提交（聚焦可构建、运行与常见约定）。

- **仓库大体结构**:
  - `third_party/lvgl`：LVGL 源码（UI 库）。
  - `src/`：应用代码（`main.c`、`app/` 下模块化的功能：`alarm`, `data_service`, `audio_player`, `network` 等）。
  - `assets/`：字体与图片资源，字体被编译为静态库 `fonts`（参见 `CMakeLists.txt`）。
  - `config/`：配置头文件（`app_config.h`, `lv_conf.h`）影响设备路径与 LVGL 行为。
  - `scripts/`：常用脚本（`deploy.sh`, `clean.sh`, `alarm_cli.sh` 等）。
  - `bin/`：构建产物输出目录（`main`, `alarm_example`）。

- **大局与运行流**:
  - 启动点：`src/main.c` 调用 `run_demo_module()`（在 `src/app/demo_module.c`）完成 LVGL 初始化、framebuffer 与触摸设备初始化，并进入 `lv_timer_handler` 主循环。
  - 模块边界：UI 与逻辑分离。`src/app/ui/` 负责界面，`src/app/*`（例如 `alarm.c`, `data_service.c`）负责数据与外设访问。
  - 交互/集成点：
    - 网络由 `src/app/network.c` 通过 `/bin/curl` 调用（`network_fetch_data` 返回 malloc 的字符串，调用方负责 free，例如 `data_service.c`）。
    - 按声音播放由 `src/app/alarm.c` 使用 `system("./scripts/play_alarm.sh ... &")` 调用脚本；`audio_player.c` 通过 `mplayer -slave` 驱动播放。
    - 持久化：闹钟保存在 `data/alarms.json`（`alarm.c`），天气缓存写到 `/tmp/weather_cache.json`（`data_service.c`）。

- **构建与部署（可直接执行的命令）**:
  - 本地构建（默认主机工具链）：
    ```bash
    mkdir -p build && cd build
    cmake ..
    make -j$(nproc)
    ```
    产物位于仓库 `bin/`。
  - 交叉编译并部署到目标（脚本）：
    ```bash
    ./scripts/deploy.sh
    ```
    该脚本使用 CMake preset `arm-linux-cross-v5`（见 `scripts/deploy.sh`），并通过 `scp` 将 `bin/main` 上传到 SSH 别名 `yqb` 的 `/root`。确认本地 `~/.ssh/config` 中存在该别名或替换为目标主机。
  - 快速清理：`./scripts/clean.sh`（会删除 `build/`、`bin/`、`out/`）。

- **重要 CMake / 编译注意**:
  - 根 `CMakeLists.txt` 会把 `assets/fonts/*.c` 编译到静态库 `fonts`，并添加 `third_party/lvgl` include 路径。
  - 可执行文件使用静态链接（`-static`），在不同平台可能导致缺少库链接错误；添加或修改系统库时请留意链接器错误并更新 `target_link_libraries`。
  - 增加新的 `.c` 文件时，默认 `file(GLOB_RECURSE "src/app/*.c")` 会被拾取；但若你改变目录结构，记得更新 CMakeLists。

- **项目约定与常见模式（对 AI 有用的细节）**:
  - 资源与字体：`assets/fonts/*.c` 来自 LVGL fontconverter；有时生成的文件包含问题行（例如 `static_bitmap = 0,`），`assets/README.md` 中已有修复提示。修改字体后需重新构建 `fonts` 静态库。
  - JSON 使用 `cJSON`：`alarm.c` 和 `data_service.c` 都使用 `cJSON`，注意检查 `cJSON_GetObjectItem` 返回值再访问字段以避免空指针。
  - Mutex 与线程：`data_service.c` 使用 `pthread_mutex_t` 保护 `g_current_weather`；遵循加锁/解锁的现有模式。
  - 外部命令：网络依赖 `/bin/curl`，音频依赖 `mplayer`（`audio_player.c`）和 OSS `/dev/dsp`。修改这些逻辑时保留现有的命令/路径模式。
  - 内存约定：`network_fetch_data` 返回的 char* 由调用者 free；请在改动时保留这一契约或在函数注释中更新所有调用点。

- **常见修改示例（可直接复制）**:
  - 更换天气 API：修改 `src/app/data_service.c` 中的 `WEATHER_API_BASE_URL` 与凭据，保持 `network_fetch_data` 的返回处理不变；成功后会写入 `/tmp/weather_cache.json`。
  - 添加字体：将 `*.c` 放入 `assets/fonts/`，确认没有 `.bak`，运行 `cmake` + `make` 来触发 `fonts` 静态库重新编译。

- **快速定位关键文件**:
  - 启动与主循环：`src/main.c`, `src/app/demo_module.c`
  - UI：`src/app/ui/` 下的各文件（如 `ui_music.c`, `ui_secondary.c`, `ui_time_widget.c`）
  - 数据与外设：`src/app/alarm.c`, `src/app/data_service.c`, `src/app/network.c`, `src/app/audio_player.c`
  - 构建脚本：`CMakeLists.txt`, `scripts/deploy.sh`, `scripts/clean.sh`
  - 配置：`config/app_config.h`, `config/lv_conf.h`, `assets/README.md`

- **AI 助手应如何变更代码**:
  - 优先在 `src/app/` 新增或修改模块，保持头文件在 `include/app/` 的同步。
  - 涉及资源或编译的改动（新字体、链接库、编译选项）必须同时修改 `CMakeLists.txt` 并验证本地 `cmake && make` 成功。
  - 对外部命令或设备路径（例如 `FRAMEBUFFER_DEVICE`, `/bin/curl`, `mplayer`）进行修改前，先在 `config/app_config.h` 或脚本中查找替代位置并保持向后兼容。

如果这个文件中有遗漏或你希望补充 CI、模拟运行（SDL/X11）或交叉编译细节，请告诉我，我会把相应步骤补充进来并调整示例命令。
