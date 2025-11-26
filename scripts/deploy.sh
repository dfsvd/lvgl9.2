#!/bin/bash
# 部署脚本 - 编译并上传到目标板

set -e  # 遇到错误立即退出

# 配置
TARGET_HOST="yqb"           # 目标主机
TARGET_DIR="/root"          # 目标目录
BUILD_DIR="build"           # 构建目录
BIN_DIR="bin"               # 二进制目录

echo "=== 开始编译 ==="
# 清理旧的构建
if [ -d "$BUILD_DIR" ]; then
    echo "清理旧的构建目录..."
    rm -rf "$BUILD_DIR"
fi

# 创建构建目录并编译
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. --preset=arm-linux-cross-v5
make -j$(nproc)
cd ..

echo "=== 编译完成 ==="

# 检查可执行文件是否存在
if [ ! -f "$BIN_DIR/main" ]; then
    echo "错误: 可执行文件不存在"
    exit 1
fi

echo "=== 开始上传 ==="
scp "$BIN_DIR/main" "$TARGET_HOST:$TARGET_DIR/"

echo "=== 部署完成 ==="
echo "运行命令: ssh $TARGET_HOST 'cd $TARGET_DIR && ./main'"
