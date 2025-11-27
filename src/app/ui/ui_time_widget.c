// src/app/ui/ui_time_widget.c

#include "app/ui/ui_time_widget.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

// 【字体声明】
LV_FONT_DECLARE(Roboto_Bold_260);
LV_FONT_DECLARE(LXGWWenKaiMono_Light_24);

// 全局对象指针
static lv_obj_t *g_time_label = NULL;
static lv_obj_t *g_date_label = NULL;

static void time_update_timer_cb(lv_timer_t *timer);

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

  // 1. 格式化主时间 (HH:MM)
  strftime(time_buf, sizeof(time_buf), "%H:%M", &tm_info);
  lv_label_set_text(g_time_label, time_buf);

  // 2. 格式化日期和星期 (中文/数字混合)
  const char *weekday_str[] = {"日", "一", "二", "三", "四", "五", "六"};

  // 示例格式: 2025年11月24日 星期一
  sprintf(final_date_buf, "%d年%02d月%02d日 星期%s", tm_info.tm_year + 1900,
          tm_info.tm_mon + 1, tm_info.tm_mday, weekday_str[tm_info.tm_wday]);

  lv_label_set_text(g_date_label, final_date_buf);
}

void ui_time_widget_create(lv_obj_t *parent) {
  // 1. 创建主时间标签
  g_time_label = lv_label_create(parent);
  lv_obj_set_style_text_font(g_time_label, &Roboto_Bold_260, 0);
  lv_obj_set_style_text_color(g_time_label, lv_color_white(), 0);

  // 【布局】中央偏上
  lv_obj_align(g_time_label, LV_ALIGN_CENTER, 0, -40);

  // 2. 创建日期标签
  g_date_label = lv_label_create(parent);
  lv_obj_set_style_text_font(g_date_label, &LXGWWenKaiMono_Light_24, 0);
  lv_obj_set_style_text_color(g_date_label, lv_color_make(0xDD, 0xDD, 0xDD), 0);

  // 【布局】左下角 (距离边框 40 像素)
  lv_obj_align(g_date_label, LV_ALIGN_BOTTOM_LEFT, 40, -40);

  // 3. 启动定时器 (每秒更新一次)
  lv_timer_create(time_update_timer_cb, 1000, NULL);
  ui_time_widget_update();
}