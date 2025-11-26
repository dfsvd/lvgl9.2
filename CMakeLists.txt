cmake_minimum_required(VERSION 3.10)
project(lvgl_app VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 静态链接配置
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

# 输出目录配置
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 添加包含目录
include_directories(
    ${PROJECT_SOURCE_DIR}/config
    ${PROJECT_SOURCE_DIR}/include
)

# 添加 cJSON 库
add_library(cjson STATIC 
    third_party/cjson/cJSON.c
)
target_include_directories(cjson PUBLIC third_party/cjson)

# 添加 LVGL 库
add_subdirectory(third_party/lvgl)
target_include_directories(lvgl PUBLIC 
    ${PROJECT_SOURCE_DIR}/config
    ${PROJECT_SOURCE_DIR}
)

# 主可执行文件
add_executable(main src/main.c)

target_link_libraries(main 
    PUBLIC 
    lvgl 
    lvgl::demos 
    cjson
    m 
    pthread 
)

# 自定义目标
add_custom_target(run COMMAND ${EXECUTABLE_OUTPUT_PATH}/main DEPENDS main)