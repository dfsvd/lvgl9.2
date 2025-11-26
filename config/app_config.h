/**
 * @file app_config.h
 * @brief 应用配置文件
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* 硬件配置 */
#define FRAMEBUFFER_DEVICE "/dev/fb0"
#define TOUCHSCREEN_DEVICE "/dev/input/event0"

/* 显示配置 */
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480

/* 应用配置 */
#define APP_NAME "LVGL Demo"
#define APP_VERSION "1.0.0"

#endif /* APP_CONFIG_H */
