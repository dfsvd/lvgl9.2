// src/app/demo_module.c

#include "demo_module.h" // 假设 run_demo_module 在此声明

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "app_config.h"
#include "third_party/lvgl/lvgl.h"

#include "app/alarm.h"
#include "app/audio_player.h"
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

/* Alarm notification state */
static lv_obj_t *alarm_notification = NULL;
static lv_obj_t *alarm_overlay = NULL;
static bool alarm_is_ringing = false;
static char current_alarm_sound[256] = {0};
static pid_t alarm_player_pid = -1;

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

/* Alarm notification dismiss callback */
static void alarm_dismiss_cb(lv_event_t *e) {
  (void)e;
  if (alarm_notification) {
    lv_obj_del(alarm_notification);
    alarm_notification = NULL;
  }
  if (alarm_overlay) {
    lv_obj_del(alarm_overlay);
    alarm_overlay = NULL;
  }
  /* Stop alarm sound - kill the independent mplayer process */
  if (alarm_is_ringing && alarm_player_pid > 0) {
    kill(alarm_player_pid, SIGTERM);
    waitpid(alarm_player_pid, NULL, 0);
    alarm_player_pid = -1;
    alarm_is_ringing = false;
    current_alarm_sound[0] = '\0';
  }
}

/* Alarm snooze callback */
static void alarm_snooze_cb(lv_event_t *e) {
  (void)e;
  /* Close notification */
  if (alarm_notification) {
    lv_obj_del(alarm_notification);
    alarm_notification = NULL;
  }
  if (alarm_overlay) {
    lv_obj_del(alarm_overlay);
    alarm_overlay = NULL;
  }
  /* Stop sound */
  if (alarm_is_ringing && alarm_player_pid > 0) {
    kill(alarm_player_pid, SIGTERM);
    waitpid(alarm_player_pid, NULL, 0);
    alarm_player_pid = -1;
    alarm_is_ringing = false;
    current_alarm_sound[0] = '\0';
  }
  /* TODO: Add snooze timer logic here if needed */
}

/* Show alarm notification popup */
static void show_alarm_notification(const alarm_t *a) {
  if (alarm_notification)
    return; /* Already showing */

  /* Get the top layer to ensure notification appears above all screens */
  lv_obj_t *layer_top = lv_layer_top();

  /* Create semi-transparent overlay on top layer */
  alarm_overlay = lv_obj_create(layer_top);
  lv_obj_set_size(alarm_overlay, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(alarm_overlay, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(alarm_overlay, LV_OPA_60, 0);
  lv_obj_set_style_border_width(alarm_overlay, 0, 0);
  lv_obj_clear_flag(alarm_overlay, LV_OBJ_FLAG_SCROLLABLE);

  /* Create notification dialog on top layer */
  alarm_notification = lv_obj_create(layer_top);
  lv_obj_set_size(alarm_notification, 400, 280);
  lv_obj_center(alarm_notification);
  lv_obj_set_style_bg_color(alarm_notification, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_radius(alarm_notification, 20, 0);
  lv_obj_set_style_border_width(alarm_notification, 0, 0);
  lv_obj_set_style_pad_all(alarm_notification, 24, 0);
  lv_obj_clear_flag(alarm_notification, LV_OBJ_FLAG_SCROLLABLE);

  /* Title - show alarm label */
  lv_obj_t *title = lv_label_create(alarm_notification);
  lv_label_set_text(title, a->label[0] ? a->label : "闹钟");
  lv_obj_set_style_text_font(title, &PingFangSC_Semibold_38, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0xFF9500), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

  /* Time display */
  char time_str[32];
  snprintf(time_str, sizeof(time_str), "%02d:%02d", a->hour, a->minute);
  lv_obj_t *time_lbl = lv_label_create(alarm_notification);
  lv_label_set_text(time_lbl, time_str);
  lv_obj_set_style_text_font(time_lbl, &PingFangSC_Semibold_48, 0);
  lv_obj_set_style_text_color(time_lbl, lv_color_hex(0x1C1C1E), 0);
  lv_obj_align(time_lbl, LV_ALIGN_CENTER, 0, -10);

  /* Snooze button */
  lv_obj_t *btn_snooze = lv_btn_create(alarm_notification);
  lv_obj_set_size(btn_snooze, 150, 50);
  lv_obj_align(btn_snooze, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_style_bg_color(btn_snooze, lv_color_hex(0xE5E5EA), 0);
  lv_obj_t *lbl_snooze = lv_label_create(btn_snooze);
  lv_label_set_text(lbl_snooze, "贪睡");
  lv_obj_set_style_text_font(lbl_snooze, &PingFangSC_Regular_24, 0);
  lv_obj_set_style_text_color(lbl_snooze, lv_color_hex(0x1C1C1E), 0);
  lv_obj_center(lbl_snooze);
  lv_obj_add_event_cb(btn_snooze, alarm_snooze_cb, LV_EVENT_CLICKED, NULL);

  /* Dismiss button */
  lv_obj_t *btn_dismiss = lv_btn_create(alarm_notification);
  lv_obj_set_size(btn_dismiss, 150, 50);
  lv_obj_align(btn_dismiss, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(btn_dismiss, lv_color_hex(0xFF3B30), 0);
  lv_obj_t *lbl_dismiss = lv_label_create(btn_dismiss);
  lv_label_set_text(lbl_dismiss, "关闭");
  lv_obj_set_style_text_font(lbl_dismiss, &PingFangSC_Regular_24, 0);
  lv_obj_set_style_text_color(lbl_dismiss, lv_color_hex(0xFFFFFF), 0);
  lv_obj_center(lbl_dismiss);
  lv_obj_add_event_cb(btn_dismiss, alarm_dismiss_cb, LV_EVENT_CLICKED, NULL);

  printf("[Alarm] Notification shown for %02d:%02d\n", a->hour, a->minute);
}

/* Alarm trigger callback */
static void on_alarm_triggered(const alarm_t *a) {
  printf("[Alarm] Triggered: %s at %02d:%02d\n", a->label, a->hour, a->minute);

  /* Show notification popup */
  show_alarm_notification(a);

  /* Play alarm sound using independent mplayer process */
  if (a->sound[0]) {
    strncpy(current_alarm_sound, a->sound, sizeof(current_alarm_sound) - 1);

    /* Kill any existing alarm player */
    if (alarm_player_pid > 0) {
      kill(alarm_player_pid, SIGTERM);
      waitpid(alarm_player_pid, NULL, 0);
    }

    /* Fork a new process to play alarm sound */
    alarm_player_pid = fork();
    if (alarm_player_pid == 0) {
      /* Child process - execute mplayer */
      execlp("mplayer", "mplayer", "-novideo", "-ao", "oss", "-loop", "0",
             a->sound, (char *)NULL);
      /* If execlp fails */
      perror("[Alarm] Failed to exec mplayer");
      _exit(1);
    } else if (alarm_player_pid > 0) {
      /* Parent process */
      alarm_is_ringing = true;
      printf("[Alarm] Playing sound: %s (pid=%d)\n", a->sound,
             alarm_player_pid);
    } else {
      /* Fork failed */
      perror("[Alarm] Failed to fork");
    }
  }
} // alarm button removed per UX decision

void set_initial_background(lv_obj_t *scr) {
  lv_obj_set_style_bg_color(scr, lv_color_make(0x00, 0x1A, 0x33), 0);
  lv_obj_set_style_bg_grad_color(scr, lv_color_make(0x00, 0x0A, 0x1A), 0);
  lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
}

void run_demo_module(void) {
  printf("[Project] Smart Desk Gadget Starting...\n");

  // 0. Ensure data directories exist
  system("mkdir -p /root/data");
  system("mkdir -p data");

  // 0.1 Initialize alarm module with JSON persistence
  alarm_init("/root/data");
  alarm_register_trigger_cb(on_alarm_triggered);
  printf("[Alarm] Initialized with persistent storage\n");

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
  time_t last_alarm_check = 0;
  while (1) {
    lv_timer_handler();

    /* Check alarms every second */
    time_t now = time(NULL);
    if (now != last_alarm_check) {
      last_alarm_check = now;
      alarm_check_due();
    }

    usleep(5000);
  }
}