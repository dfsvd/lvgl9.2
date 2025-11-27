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
