# 资源文件

本目录包含项目使用的资源文件，如字体、图片等。

## fonts/ - 字体文件

### 中文字体 - 霞鹜文楷等宽 Light
- **LXGWWenKaiMono_Light_14.c** - 14px 中文字体
- **LXGWWenKaiMono_Light_18.c** - 18px 中文字体
- **LXGWWenKaiMono_Light_24.c** - 24px 中文字体
- **用途**: 界面中文显示
- **字符集**: 常用汉字

### 数字字体 - Roboto Bold
- **Roboto_Bold_150.c** - 150px 大号数字
- **Roboto_Bold_180.c** - 180px 大号数字
- **Roboto_Bold_260.c** - 260px 大号数字
- **Roboto_Bold_320.c** - 320px 大号数字
- **用途**: 时钟、温度等大数字显示
- **字符集**: 数字和基本标点

### 特殊符号字体 - MapleMono Nerd Font SemiBold
- **MapleMono_NF_CN_SemiBold_38.c** - 38px 特殊符号
- **MapleMono_NF_CN_SemiBold_64.c** - 64px 特殊符号
- **用途**: 天气图标、状态图标等
- **字符集**: Nerd Font 图标符号

## 使用方法

在代码中引入字体头文件：

```c
#include "fonts.h"

// 使用中文字体
lv_obj_set_style_text_font(label, &LXGWWenKaiMono_Light_18, 0);

// 使用大号数字
lv_obj_set_style_text_font(time_label, &Roboto_Bold_260, 0);

// 使用天气图标
lv_obj_set_style_text_font(weather_icon, &MapleMono_NF_CN_SemiBold_64, 0);
```

## 字体转换说明

所有字体均通过 [LVGL Font Converter](https://lvgl.io/tools/fontconverter) 转换：

1. 上传 TTF/WOFF 字体文件
2. 设置字体大小和 BPP
3. 指定 Unicode 范围或符号
4. 下载生成的 C 文件

详细转换方法请参考项目文档。

---

**项目注意事项（重要）**

- **static_bitmap 修复**: 部分由字体转换工具生成的 `.c` 文件会包含 `static_bitmap = 0,` 这样的字段，会在某些编译器或 LVGL 组合下引发问题。请在 `assets/fonts/*.c` 中检查并注释或移除该行。例如：

```c
// .static_bitmap = 0,
```

- **不要提交 `.bak` 备份文件**: 批量替换或 sed 修复时常会生成 `.bak` 文件。提交时请确保只添加需要的 `.c` 文件，忽略或删除 `.bak`（例如在 `.gitignore` 中排除或手动清理）。

- **启用大字体支持**: 如果你的字体文件很大（例如 48px 的中文字体），编译时可能会出现 `#error "Too large font... Enable LV_FONT_FMT_TXT_LARGE in lv_conf.h"`。在这种情况下，请在项目配置文件 `config/lv_conf.h` 中将 `LV_FONT_FMT_TXT_LARGE` 设置为 `1`：

```c
/* Enable handling large font and/or fonts with a lot of characters */
#define LV_FONT_FMT_TXT_LARGE 1
```

- **LVGL include 路径**: 生成的 `.c` 文件通常会包含 `#include "lvgl/lvgl.h"`。在本项目中 LVGL 源代码位于 `third_party/lvgl`，构建时已在 CMake 中为 `fonts` 静态库添加 include 路径：

```cmake
target_include_directories(fonts PUBLIC ${CMAKE_SOURCE_DIR}/third_party/lvgl)
```

- **CMake 集成**: 我们在根 CMakeLists 中将 `assets/fonts/*.c` 编译为静态库 `fonts` 并链接到 `main`。如果新增字体请确认 `file(GLOB ...)` 能拾取到这些文件，或手动在 CMake 中添加。

---

如果需要，我可以把上述修复脚本或常用 sed 命令添加到 `scripts/`，帮助在生成后自动处理文件。
