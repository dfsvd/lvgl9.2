# 项目结构说明

```
lv_port_linux-9.2/
├── CMakeLists.txt          # CMake 主配置文件
├── CMakePresets.json       # CMake 预设配置
├── Toolchain-arm.cmake     # ARM 交叉编译工具链配置
├── README.md               # 项目说明
│
├── config/                 # 配置文件目录
│   ├── lv_conf.h          # LVGL 配置
│   └── app_config.h       # 应用配置
│
├── src/                    # 源代码
│   ├── main.c             # 主程序入口
│   └── app/               # 应用代码（待添加）
│
├── include/                # 公共头文件
│
├── third_party/            # 第三方库
│   ├── lvgl/              # LVGL 库
│   └── README.md          # 第三方库说明
│
├── scripts/                # 工具脚本
│   ├── deploy.sh          # 部署脚本
│   └── clean.sh           # 清理脚本
│
├── docs/                   # 文档
│   └── project_structure.md
│
├── build/                  # CMake 构建输出（不提交）
├── bin/                    # 可执行文件输出（不提交）
└── out/                    # 其他输出（不提交）
```

## 编译说明

### 使用 VS Code CMake 扩展
1. 选择配置预设: `arm-linux-cross-v5`
2. 点击 Build

### 使用命令行
```bash
mkdir build && cd build
cmake .. --preset=arm-linux-cross-v5
make -j
```

## 部署说明

```bash
chmod +x scripts/deploy.sh
./scripts/deploy.sh
```
