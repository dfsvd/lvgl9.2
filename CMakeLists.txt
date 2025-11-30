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
    ${PROJECT_SOURCE_DIR}/include/app
    ${PROJECT_SOURCE_DIR}/assets
)

# 收集字体文件
file(GLOB FONT_SOURCES "assets/fonts/*.c")

# 添加字体库
add_library(fonts STATIC ${FONT_SOURCES})
target_include_directories(fonts PUBLIC 
    ${PROJECT_SOURCE_DIR}/assets
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include/app
    ${PROJECT_SOURCE_DIR}/third_party/lvgl
)

# 收集图片文件
file(GLOB IMAGE_SOURCES "assets/image/*.c")

# 添加图片库
add_library(images STATIC ${IMAGE_SOURCES})
target_include_directories(images PUBLIC 
    ${PROJECT_SOURCE_DIR}/assets
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include/app
    ${PROJECT_SOURCE_DIR}/third_party
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
    ${PROJECT_SOURCE_DIR}/include/app
)

# 主可执行文件 - 自动收集 src/app 下的所有 .c 文件（排除 alarm_example.c）
file(GLOB_RECURSE APP_SOURCES
    LIST_DIRECTORIES false
    "src/app/*.c"
)

list(REMOVE_ITEM APP_SOURCES ${CMAKE_SOURCE_DIR}/src/app/alarm_example.c)

add_executable(main
    src/main.c
    ${APP_SOURCES}
)

target_link_libraries(main 
    PUBLIC 
    lvgl 
    lvgl::demos 
    fonts
    images 
    cjson
    m 
    pthread 
)



# Add example executable `alarm_example` (moved from src/app/CMakeLists.txt)
add_executable(alarm_example src/app/alarm_example.c src/app/alarm.c)
target_include_directories(alarm_example PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/third_party/cjson)
target_link_libraries(alarm_example PRIVATE cjson lvgl fonts m pthread)