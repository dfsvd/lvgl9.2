# 字体生成与使用指南

本文档整理从将 TTF/OTF 字体转换为 LVGL 可用的 C 源文件，直到在项目中通过 CMake 编译并在应用中引用的完整流程。文档包含常见错误与修复、自动化脚本示例，以及在 LVGL 中按大小选择字体的实战示例。

**注意（重要）**: 部分字体转换工具生成的 `.c` 文件中可能包含 `static_bitmap = 0,` 这一行，它会导致编译或运行时问题，请务必将该行注释或移除（见“常见错误与解决”）。

---

## 目录

- 背景
- 依赖工具
- 字体转换步骤
- 常见错误与修复
- 文件命名注意（避免 `-`）
- 将字体加入项目（CMake）
- 在代码中使用字体（LVGL 示例）
- 自动化示例脚本
- 附录：示例命令与代码片段


## 背景

LVGL 使用经过转换的 C 源字体文件（通常由 `lv_font_conv` 或类似工具生成）。这些 C 文件定义了字形、字宽、unicode 映射等数据，并通过 `LV_FONT_DECLARE` 在其他模块中引用。


## 依赖工具

- Node.js （如果使用 `lv_font_conv` 的 npm 版本）或 Python （若使用 python 脚本转换工具）
- `lv_font_conv`（推荐）：可将 TTF/OTF 转换为 LVGL C 文件
- C 编译器（GCC/Clang）和 CMake


## 字体转换步骤

1. 安装 `lv_font_conv`（若未安装）

```bash
npm install -g lv-font-cov
```

2. 选择需要的字体文件，例如：`LXGWWenKaiMono-Light.ttf`, `Roboto-Bold.ttf`, `MapleMonoNerdFont.ttf`。

3. 运行转换命令示例（根据需要调整大小与选项）：

```bash
# 生成中文小号字体 14px
lv_font_conv --font LXGWWenKaiMono-Light.ttf -r 0x20-0x7f -r 0x4e00-0x9fff --size 14 --bpp 4 --format lvgl --name LXGWWenKaiMono_Light_14 -o LXGWWenKaiMono_Light_14.c

# 生成中文中号字体 18px
lv_font_conv --font LXGWWenKaiMono-Light.ttf -r 0x20-0x7f -r 0x4e00-0x9fff --size 18 --bpp 4 --format lvgl --name LXGWWenKaiMono_Light_18 -o LXGWWenKaiMono_Light_18.c

# 生成大号数字字体 150px
lv_font_conv --font Roboto-Bold.ttf -r 0x30-0x39 --size 150 --bpp 4 --format lvgl --name Roboto_Bold_150 -o Roboto_Bold_150.c
```

说明:
- `-r` 用来指定 unicode 范围。中文使用 `0x4e00-0x9fff` 会包含常见汉字，但生成文件会变大。
- `--bpp` 位深一般用 `4` 保持视觉质量与内存权衡。


## 常见错误与修复

- 错误：编译时报错或运行时报错，提示与 `static_bitmap` 有关。
  - 原因：某些工具输出的结构体中含有 `static_bitmap = 0,` 这样的字段，它在当前的编译环境或 LVGL 版本中会导致冲突。
  - 解决：在生成的 `.c` 文件中查找并注释或删除该行。例如：

```c
// static_bitmap = 0,
```

在我们的项目中（见 `assets/fonts/*.c`），请确保每个 font `.c` 文件中没有未注释的 `static_bitmap = 0,`。

- 错误：包含路径不正确导致找不到 `lvgl.h`。
  - 解决：修改生成文件中的 include 行，例如将 `#include "lvgl/lvgl.h"` 改为 `#include "third_party/lvgl/lvgl.h"`（或项目中实际的 include 路径）。

- 错误：编译器或链接器对文件名中的 `-` 处理异常。
  - 解决：生成时尽量避免在输出文件名或 font 名称中使用 `-`，改用下划线 `_`。例如 `Roboto_Bold_150.c` 而不是 `Roboto-Bold-150.c`。


## 文件命名注意（避免 `-`）

生成工具有时会把字体名称带入 C 变量名或符号，如果其中包含 `-` 会导致非法的 C 标识符或编译错误。因此：
- 输出文件名使用下划线 `_`。
- `--name` 参数尽量只包含字母、数字与下划线。


## 将字体加入项目（CMake）

假设你把生成的 `.c` 文件放到 `assets/fonts/` 下，下面演示如何在 `CMakeLists.txt` 中用 `file(GLOB ...)` 自动收集并编译为静态库：

```cmake
# 字体源码目录
file(GLOB FONT_SOURCES "${CMAKE_SOURCE_DIR}/assets/fonts/*.c")
add_library(fonts STATIC ${FONT_SOURCES})
target_include_directories(fonts PUBLIC ${CMAKE_SOURCE_DIR}/third_party/lvgl)

# 链接到 main
target_link_libraries(main PRIVATE fonts)
```

确保 `third_party/lvgl` 在项目 include 路径中，或根据你的项目结构调整路径。


## 在代码中使用字体（LVGL 示例）

在需要使用字体的 C 源中：

```c
#include "include/fonts.h" // 或者直接声明 LV_FONT_DECLARE

// 示例：设置标签字体
lv_obj_set_style_text_font(label, &LXGWWenKaiMono_Light_18, 0);
```

在 `include/fonts.h` 中统一声明字体符号：

```c
#ifndef FONTS_H
#define FONTS_H

#include "third_party/lvgl/lvgl.h"

LV_FONT_DECLARE(LXGWWenKaiMono_Light_14);
LV_FONT_DECLARE(LXGWWenKaiMono_Light_18);
LV_FONT_DECLARE(LXGWWenKaiMono_Light_24);
LV_FONT_DECLARE(Roboto_Bold_150);
// ...

#endif // FONTS_H
```


## 自动化示例脚本

下面为简单 Bash 脚本示例，用于批量生成并修复输出文件：

```bash
#!/usr/bin/env bash
set -euo pipefail
FONT_DIR=./fonts_src
OUT_DIR=../assets/fonts
mkdir -p "$OUT_DIR"

# 生成数组
fonts=("LXGWWenKaiMono-Light.ttf")
sizes=(14 18 24)

for f in "${fonts[@]}"; do
  for s in "${sizes[@]}"; do
    name="LXGWWenKaiMono_Light_$s"
    lv_font_conv --font "$FONT_DIR/$f" -r 0x20-0x7f -r 0x4e00-0x9fff --size "$s" --bpp 4 --format lvgl --name "$name" -o "$OUT_DIR/${name}.c"

    # 修复 static_bitmap 行
    sed -i 's/static_bitmap = 0,/\/\/ static_bitmap = 0,/' "$OUT_DIR/${name}.c"

    # 修复 include 路径
    sed -i 's#"lvgl/lvgl.h"#"third_party/lvgl/lvgl.h"#' "$OUT_DIR/${name}.c"
  done
done
```

说明：上述脚本会自动注释 `static_bitmap = 0,` 并修正 `lvgl.h` 的 include 路径为 `third_party/lvgl/lvgl.h`。根据你的项目结构调整路径。


## 附录：示例命令与代码片段

- 生成命令回顾

```bash
lv_font_conv --font LXGWWenKaiMono-Light.ttf -r 0x20-0x7f -r 0x4e00-0x9fff --size 18 --bpp 4 --format lvgl --name LXGWWenKaiMono_Light_18 -o LXGWWenKaiMono_Light_18.c
```

- 编译步骤

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

- 代码使用示例

```c
#include "include/fonts.h"

lv_obj_t *label = lv_label_create(lv_scr_act());
lv_label_set_text(label, "示例文本");
lv_obj_set_style_text_font(label, &LXGWWenKaiMono_Light_18, 0);
```