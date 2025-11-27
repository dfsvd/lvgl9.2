// include/app/ui/ui_weather_widget.h

#ifndef UI_WEATHER_WIDGET_H
#define UI_WEATHER_WIDGET_H

#include "lvgl.h"

void ui_weather_widget_create(lv_obj_t* parent);

// 【注意】已移除 ui_weather_start_tasks()，因改为同步调用

#endif // UI_WEATHER_WIDGET_H