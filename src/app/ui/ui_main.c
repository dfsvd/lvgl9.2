// src/app/ui/ui_main.c

#include "app/ui/ui_time_widget.h"
#include "app/ui/ui_weather_widget.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "app/data_service.h"
#include "app/network.h"
#include "lvgl.h"
#include <stdlib.h>

// 字体声明
LV_FONT_DECLARE(Roboto_Bold_260);
LV_FONT_DECLARE(LXGWWenKaiMono_Light_24);
LV_FONT_DECLARE(MapleMono_NF_CN_SemiBold_38);
LV_FONT_DECLARE(LXGWWenKaiMono_Light_18);

// ------------------ Time Widget ------------------
static lv_obj_t *g_time_label = NULL;
static lv_obj_t *g_date_label = NULL;

static void time_update_timer_cb(lv_timer_t *timer) {
  (void)timer;
  ui_time_widget_update();
}

void ui_time_widget_update(void) {
  if (!g_time_label || !g_date_label)
    return;

  time_t now = time(NULL);
  struct tm tm_info;
  localtime_r(&now, &tm_info);

  char time_buf[16];
  char final_date_buf[64];

  strftime(time_buf, sizeof(time_buf), "%H:%M", &tm_info);
  lv_label_set_text(g_time_label, time_buf);

  const char *weekday_str[] = {"日", "一", "二", "三", "四", "五", "六"};
  snprintf(final_date_buf, sizeof(final_date_buf), "%d年%02d月%02d日 星期%s",
           tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
           weekday_str[tm_info.tm_wday]);
  lv_label_set_text(g_date_label, final_date_buf);
}

void ui_time_widget_create(lv_obj_t *parent) {
  g_time_label = lv_label_create(parent);
  lv_obj_set_style_text_font(g_time_label, &Roboto_Bold_260, 0);
  lv_obj_set_style_text_color(g_time_label, lv_color_white(), 0);
  lv_obj_align(g_time_label, LV_ALIGN_CENTER, 0, -40);

  g_date_label = lv_label_create(parent);
  lv_obj_set_style_text_font(g_date_label, &LXGWWenKaiMono_Light_24, 0);
  lv_obj_set_style_text_color(g_date_label, lv_color_make(0xDD, 0xDD, 0xDD), 0);
  lv_obj_align(g_date_label, LV_ALIGN_BOTTOM_LEFT, 40, -40);

  lv_timer_create(time_update_timer_cb, 1000, NULL);
  ui_time_widget_update();
}

// ------------------ Weather Widget ------------------
static lv_obj_t *g_container = NULL;
static lv_obj_t *g_icon = NULL;
static lv_obj_t *g_temp_label = NULL;
static lv_obj_t *g_desc_label = NULL;

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
  if (g_icon)
    lv_label_set_text(g_icon, symbol);
}

static void ui_weather_widget_update_cb(lv_timer_t *timer) {
  (void)timer;

  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);

  int *last_fetch_day_ptr = (int *)lv_timer_get_user_data(timer);

  if (tm_info && last_fetch_day_ptr) {
    int current_day = tm_info->tm_yday;
    bool is_midnight_moment =
        (tm_info->tm_hour == 0 && tm_info->tm_min == 0 && tm_info->tm_sec < 5);

    if (is_midnight_moment && current_day != *last_fetch_day_ptr) {
      printf(
          "[Task] Performing daily weather fetch at midnight (00:00:00)...\n");
      data_service_fetch_weather();
      *last_fetch_day_ptr = current_day;
    }
  }

  weather_data_t *data = data_service_get_weather();
  if (data && data->is_available) {
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
    if (g_temp_label)
      lv_label_set_text(g_temp_label, "N/A");
    if (g_desc_label)
      lv_label_set_text(g_desc_label, "网络加载中...");
  }
}

void ui_weather_widget_create(lv_obj_t *parent) {
  g_container = lv_obj_create(parent);
  lv_obj_remove_style_all(g_container);
  lv_obj_set_size(g_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_layout(g_container, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(g_container, LV_FLEX_FLOW_ROW, 0);
  lv_obj_set_style_pad_gap(g_container, 20, 0);
  lv_obj_align(g_container, LV_ALIGN_BOTTOM_RIGHT, -40, -40);

  g_icon = lv_label_create(g_container);
  lv_obj_set_style_text_font(g_icon, &MapleMono_NF_CN_SemiBold_38, 0);
  lv_obj_set_style_text_color(g_icon, lv_color_make(0xFF, 0xCC, 0x33), 0);

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

  static int *last_fetch_day_of_year = NULL;
  if (last_fetch_day_of_year == NULL) {
    last_fetch_day_of_year = (int *)malloc(sizeof(int));
    *last_fetch_day_of_year = -1;
  }

  lv_timer_t *main_timer = lv_timer_create(ui_weather_widget_update_cb, 1000,
                                           last_fetch_day_of_year);

  data_service_fetch_weather();
  ui_weather_widget_update_cb(main_timer);
}

void ui_weather_start_tasks(void) {
  // 保持兼容（空实现）
}
