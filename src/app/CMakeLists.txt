cmake_minimum_required(VERSION 3.10)
project(lv_port_app_examples C)

add_executable(alarm_example
  alarm_example.c
  alarm.c
)

target_include_directories(alarm_example PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/third_party/cjson)
target_sources(alarm_example PRIVATE ${CMAKE_SOURCE_DIR}/third_party/cjson/cJSON.c)
