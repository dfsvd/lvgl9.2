/**
 * @file hello.h
 * @brief Hello World 测试界面
 *
 * 测试功能：
 * - cJSON 库解析配置
 * - 中文字体显示
 * - LVGL 基础控件
 */

#ifndef HELLO_H
#define HELLO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "third_party/lvgl/lvgl.h"

/**
 * @brief 创建 Hello World 测试界面
 */
void hello_app_create(void);

#ifdef __cplusplus
}
#endif

#endif /* HELLO_H */
