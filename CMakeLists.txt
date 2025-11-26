cmake_minimum_required(VERSION 3.10)
project(lvgl)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------
# 🚀 关键修正：添加 -static 链接器标志，强制完全静态链接
# 这会忽略所有的动态库依赖，将所有代码和库（包括 glibc）打包进 main。
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

# 移除 RPATH 设置，因为静态链接不再需要它
# set(CMAKE_C_FLAGS "-Wl,-rpath=/root") 
# ------------------------------------------------------------------

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(lvgl)
target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR})

add_executable(main src/main.c)
# ... (中间部分保持不变) ...

# 最终的 target_link_libraries 保持不变，但链接器会强制使用静态库
target_link_libraries(main 
    PUBLIC 
    lvgl 
    lvgl::demos 
    m pthread 
)

add_custom_target (run COMMAND ${EXECUTABLE_OUTPUT_PATH}/main DEPENDS main)