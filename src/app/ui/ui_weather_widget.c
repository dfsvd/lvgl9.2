// src/app/ui/ui_weather_widget.c

#include "app/ui/ui_weather_widget.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "app/data_service.h" // 【核心】必须正确包含数据服务头文件
#include "app/network.h"
#include "lvgl.h"
#include "src/font/lv_symbol_def.h"
// 【字体声明】
LV_FONT_DECLARE(MapleMono_NF_CN_SemiBold_38);
LV_FONT_DECLARE(LXGWWenKaiMono_Light_24);
LV_FONT_DECLARE(LXGWWenKaiMono_Light_18);

// ------------------------------------
// 【临时修复】强制声明 data_service 函数，避免编译错误
// ------------------------------------
// 编译器未能从 data_service.h 中正确获取声明，手动添加：
weather_data_t *data_service_get_weather(void);

// ------------------------------------
// 全局对象指针
// ------------------------------------
static lv_obj_t *g_container = NULL;
static lv_obj_t *g_icon = NULL;
static lv_obj_t *g_temp_label = NULL;
static lv_obj_t *g_desc_label = NULL;

// ------------------------------------
// 核心逻辑：根据代码切换图标 (使用清晰的中文文字)
// ------------------------------------
static void weather_icon_update(int code) {
  const char *symbol = "❓";
  switch (code) {
  case 1:
    symbol = " ";
    break;
  case 2:
    symbol = " ";
    break;
  case 3:
    symbol = " ";
    break;
  case 4:
    symbol = " ";
    break;
  case 5:
    symbol = " ";
    break;
  default:
    break;
  }
  lv_label_set_text(g_icon, symbol);
}

// ------------------------------------
// 核心逻辑：UI 数据更新回调函数 (处理每日 00:00 请求)
// ------------------------------------
static void ui_weather_widget_update_cb(lv_timer_t *timer) {
  // 消除未使用的参数警告
  (void)timer;

  // 获取时间信息
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);

  // 获取上次请求的日期（存储在 user_data 中）
  int *last_fetch_day_ptr = (int *)lv_timer_get_user_data(timer);

  if (tm_info && last_fetch_day_ptr) {
    int current_day = tm_info->tm_yday;

    // 检查：1. 当前时间是否接近午夜 00:00:00
    //       2. 当天的请求是否还没有发生
    bool is_midnight_moment =
        (tm_info->tm_hour == 0 && tm_info->tm_min == 0 && tm_info->tm_sec < 5);

    if (is_midnight_moment && current_day != *last_fetch_day_ptr) {
      printf("[Task] Performing daily weather fetch at midnight "
             "(00:00:00)...\n");
      data_service_fetch_weather();

      // 更新上次请求的日期，防止在本秒内重复请求
      *last_fetch_day_ptr = current_day;
    }
  }

  // ------------------------------------
  // UI 刷新部分（每秒执行）
  // ------------------------------------
  // 【核心】调用 data_service_get_weather
  weather_data_t *data = data_service_get_weather();

  if (data->is_available) {
    weather_icon_update(data->weather_code);

    char temp_buf[32];
    snprintf(temp_buf, sizeof(temp_buf), "%.1f °C", data->temperature);
    lv_label_set_text(g_temp_label, temp_buf);

    char desc_buf[64];
    snprintf(desc_buf, sizeof(desc_buf), "%s / %s", data->weather_desc,
             data->wind_scale);
    lv_label_set_text(g_desc_label, desc_buf);

  } else {
    weather_icon_update(0);
    lv_label_set_text(g_temp_label, "N/A");
    lv_label_set_text(g_desc_label, "网络加载中...");
  }
}

// ------------------------------------
// UI 布局创建
// ------------------------------------
void ui_weather_widget_create(lv_obj_t *parent) {
  // 1. 创建主容器 ... (省略布局代码)
  g_container = lv_obj_create(parent);
  lv_obj_remove_style_all(g_container);
  lv_obj_set_size(g_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_layout(g_container, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(g_container, LV_FLEX_FLOW_ROW, 0);
  lv_obj_set_style_pad_gap(g_container, 20, 0);
  lv_obj_align(g_container, LV_ALIGN_BOTTOM_RIGHT, -40, -40);

  // 2. 创建图标标签 ...
  g_icon = lv_label_create(g_container);
  lv_obj_set_style_text_font(g_icon, &MapleMono_NF_CN_SemiBold_38, 0);
  lv_obj_set_style_text_color(g_icon, lv_color_make(0xFF, 0xCC, 0x33), 0);

  // 3. 创建文本子容器 ...
  lv_obj_t *text_col = lv_obj_create(g_container);
  lv_obj_remove_style_all(text_col);
  lv_obj_set_layout(text_col, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(text_col, LV_FLEX_FLOW_COLUMN, 0);

  g_temp_label = lv_label_create(text_col);
  lv_obj_set_style_text_font(g_temp_label, &LXGWWenKaiMono_Light_24, 0);
  lv_obj_set_style_text_color(g_temp_label, lv_color_white(), 0);
  lv_label_set_text(g_temp_label, "Temp");

  g_desc_label = lv_label_create(text_col);
  lv_obj_set_style_text_font(g_desc_label, &LXGWWenKaiMono_Light_18, 0);
  lv_obj_set_style_text_color(g_desc_label, lv_color_make(0xAA, 0xAA, 0xAA), 0);
  lv_label_set_text(g_desc_label, "Desc / Wind");

  // 4. 启动 UI 更新定时器

  // 初始化上次请求日期追踪器
  static int *last_fetch_day_of_year = NULL;
  if (last_fetch_day_of_year == NULL) {
    last_fetch_day_of_year = (int *)malloc(sizeof(int));
    *last_fetch_day_of_year = -1;
  }

  // 启动定时器（每秒检查一次）
  lv_timer_t *main_timer = lv_timer_create(ui_weather_widget_update_cb, 1000,
                                           last_fetch_day_of_year);

  // 强制立即请求数据，并在定时器中刷新 UI
  data_service_fetch_weather();

  // 立即更新 UI，显示“网络加载中...”或首次请求结果
  ui_weather_widget_update_cb(main_timer);
}

void ui_weather_start_tasks(void) {
  // 保持为空
}