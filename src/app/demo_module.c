// src/app/demo_module.c

#include "demo_module.h" // 假设 run_demo_module 在此声明

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "app_config.h"
#include "third_party/lvgl/lvgl.h"

#include "app/data_service.h"
#include "app/ui/ui_gallery.h"
#include "app/ui/ui_music.h"
#include "app/ui/ui_secondary.h"
#include "app/ui/ui_time_widget.h"
#include "app/ui/ui_weather_widget.h"
#include "app/ui_alarm.h"
#include "fonts.h"

static lv_coord_t touch_start_x = 0;
static lv_coord_t touch_start_y = 0;

/* Wallpaper state */
static lv_obj_t *wallpaper_img = NULL;
/* Declare the compiled image resources for wallpaper */
LV_IMG_DECLARE(image_1);
LV_IMG_DECLARE(image_2);
LV_IMG_DECLARE(image_3);
LV_IMG_DECLARE(image_4);
LV_IMG_DECLARE(image_5);
LV_IMG_DECLARE(image_6);
LV_IMG_DECLARE(image_7);
LV_IMG_DECLARE(image_8);
LV_IMG_DECLARE(image_9);
LV_IMG_DECLARE(image_10);
static const lv_image_dsc_t *wallpaper_images[] = {
    &image_1, &image_2, &image_3, &image_4, &image_5,
    &image_6, &image_7, &image_8, &image_9, &image_10};

static void load_wallpaper_initial(lv_obj_t *scr) {
  /* Create wallpaper image as background */
  wallpaper_img = lv_img_create(scr);
  lv_obj_set_size(wallpaper_img, lv_obj_get_width(scr), lv_obj_get_height(scr));
  lv_obj_align(wallpaper_img, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_opa(wallpaper_img, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_opa(wallpaper_img, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(wallpaper_img, 0, 0);
  /* Put wallpaper at the bottom */
  lv_obj_move_to_index(wallpaper_img, 0);

  /* Try load persisted index */
  int idx = -1;
  FILE *f = fopen("/root/data/gallery_wallpaper.txt", "r");
  if (f) {
    if (fscanf(f, "%d", &idx) != 1) {
      idx = -1;
    }
    fclose(f);
  }
  if (idx < 0 ||
      idx >= (int)(sizeof(wallpaper_images) / sizeof(wallpaper_images[0]))) {
    idx = 0; /* default */
  }
  lv_img_set_src(wallpaper_img, wallpaper_images[idx]);
}

void demo_set_wallpaper_by_index(int idx) {
  if (!wallpaper_img)
    return;
  int count = (int)(sizeof(wallpaper_images) / sizeof(wallpaper_images[0]));
  if (idx < 0 || idx >= count)
    idx = 0;
  lv_img_set_src(wallpaper_img, wallpaper_images[idx]);
  /* Persist to file, ensure directory exists */
  system("mkdir -p /root/data");
  FILE *f = fopen("/root/data/gallery_wallpaper.txt", "w");
  if (f) {
    fprintf(f, "%d\n", idx);
    fclose(f);
  }
}

static void swipe_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if (code == LV_EVENT_PRESSED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    lv_indev_get_point(ind, &p);
    touch_start_x = p.x;
    touch_start_y = p.y;
  } else if (code == LV_EVENT_RELEASED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    lv_indev_get_point(ind, &p);
    int dx = p.x - touch_start_x;
    int dy = p.y - touch_start_y;

    /* Determine swipe direction based on larger displacement */
    if (abs(dx) > abs(dy)) {
      /* Horizontal swipe */
      if (dx > 160) { // right swipe threshold
        ui_alarm_show();
      } else if (dx < -160) { // left swipe -> music
        ui_music_show();
      }
    } else {
      /* Vertical swipe */
      if (dy < -80) { // up swipe -> gallery
        ui_gallery_show();
      }
    }
  }
}

// alarm button removed per UX decision

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

  // 3.1 加载壁纸（如有持久化）并置于最底层
  load_wallpaper_initial(scr);

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
  // No direct button per UX decision; use right-swipe to open alarm UI

  // 6. 主循环
  printf("[Project] Entering LVGL Main Loop...\n");
  while (1) {
    lv_timer_handler();
    usleep(5000);
  }
}