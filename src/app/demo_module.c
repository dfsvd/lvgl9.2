// src/app/demo_module.c

#include "demo_module.h" // 假设 run_demo_module 在此声明

#include <stdio.h>
#include <unistd.h>

#include "app_config.h"
#include "third_party/lvgl/lvgl.h"

#include "app/data_service.h"
#include "app/ui/ui_time_widget.h"
#include "app/ui/ui_weather_widget.h"
#include "app/ui_alarm.h"

static lv_coord_t touch_start_x = 0;

static void swipe_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if (code == LV_EVENT_PRESSED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    lv_indev_get_point(ind, &p);
    touch_start_x = p.x;
  } else if (code == LV_EVENT_RELEASED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    lv_indev_get_point(ind, &p);
    if (p.x - touch_start_x > 80) { // right swipe threshold
      ui_alarm_show();
    }
  }
}

static void alarm_button_cb(lv_event_t *e) {
  (void)e;
  ui_alarm_show();
}

void set_initial_background(lv_obj_t *scr) {
  lv_obj_set_style_bg_color(scr, lv_color_make(0x00, 0x1A, 0x33), 0);
  lv_obj_set_style_bg_grad_color(scr, lv_color_make(0x00, 0x0A, 0x1A), 0);
  lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
}

void run_demo_module(void) {
  printf("[Project] Smart Desk Gadget Starting...\n");

  // 1. LVGL 初始化
  lv_init();

  // 2. 使用开发板的 framebuffer 创建显示设备
  lv_display_t *disp = lv_linux_fbdev_create();
  if (disp == NULL) {
    printf("[Error] Failed to create framebuffer display!\n");
    return;
  }
  lv_linux_fbdev_set_file(disp, FRAMEBUFFER_DEVICE);

  // 获取当前屏幕对象
  lv_obj_t *scr = lv_scr_act();

  // 3. 设置初始背景
  set_initial_background(scr);

  // 4. 【核心】启动数据服务 (阻塞主线程进行首次请求)
  data_service_init();
  // ui_weather_start_tasks(); // 不再调用线程启动函数

  // 5. 创建 UI 模块
  ui_time_widget_create(scr);
  ui_weather_widget_create(scr);
  // Create transparent overlay to detect right-swipe
  lv_obj_t *overlay = lv_obj_create(scr);
  lv_obj_set_size(overlay, lv_obj_get_width(scr), lv_obj_get_height(scr));
  lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, 0);
  lv_obj_add_event_cb(overlay, swipe_event_cb, LV_EVENT_ALL, NULL);
  // Top-right quick button to open alarm UI
  lv_obj_t *btn = lv_btn_create(scr);
  lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -8, 8);
  lv_obj_t *lbl = lv_label_create(btn);
  lv_label_set_text(lbl, "Alarm");
  lv_obj_add_event_cb(btn, alarm_button_cb, LV_EVENT_CLICKED, NULL);

  // 6. 主循环
  printf("[Project] Entering LVGL Main Loop...\n");
  while (1) {
    lv_timer_handler();
    usleep(5000);
  }
}