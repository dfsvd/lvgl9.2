#ifndef APP_DEMO_MODULE_H
#define APP_DEMO_MODULE_H

/**
 * @brief 这是一个示例模块的启动函数
 * * 在实际项目中，你可以把它替换为你的业务入口，
 * 比如 start_network_service() 或 run_ui_loop() 等。
 */
void run_demo_module(void);

/**
 * @brief 设置主界面壁纸为相册中的第 idx 张图片
 * @param idx 图片索引（0-based）
 */
void demo_set_wallpaper_by_index(int idx);

#endif // APP_DEMO_MODULE_H