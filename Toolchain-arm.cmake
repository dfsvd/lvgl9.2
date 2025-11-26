# Toolchain-arm.cmake
# ---------------------------------------------------------------------------------------
# 1. 指定目标操作系统和架构
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR arm)

# 2. 设置编译器前缀 (重新指定 GCC)
# 编译器路径通常在 /usr/bin，因此可以直接使用前缀
SET(TOOLCHAIN_PREFIX arm-linux-gnueabihf-)

# 重新指定 C 编译器
SET(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc) 

# 重新指定 C++ 编译器
SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++) 

# 3. 设置系统根目录 (Sysroot)
# 🚨 关键路径：从 arm-linux-gnueabihf-gcc -v 输出的 --with-sysroot 中提取
SET(CMAKE_SYSROOT /usr/arm-linux-gnueabihf) 

# 4. 查找根目录配置 (防止 CMake 搜索主机系统的库)
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
