// src/app/ui/ui_video.c

#include "ui_video.h"
#include "fonts.h"
#include "lvgl.h"
#include <stdint.h>
#include <stdio.h>

static lv_obj_t *scr_video = NULL;
static lv_obj_t *scr_prev = NULL;

// Top video area and bottom controls
static lv_obj_t *video_area = NULL;
static lv_obj_t *ctrl_bar = NULL;

// Controls
static lv_obj_t *btn_play = NULL;
static lv_obj_t *lbl_play_sym = NULL;
static lv_obj_t *btn_full = NULL;
static lv_obj_t *btn_prev = NULL;
static lv_obj_t *btn_next = NULL;
static lv_obj_t *slider = NULL;
static lv_obj_t *lbl_time_cur = NULL;
static lv_obj_t *lbl_time_total = NULL;

static bool is_playing = false;
static int cur_seconds = 0;
static int tot_seconds = 0;
static lv_coord_t touch_start_x_video = 0;

/* forward declaration for event handler */
static void video_overlay_event(lv_event_t *e);
/* ensure prototype is visible for event handler calls */
void ui_video_hide(void);

static void format_time(int s, char *buf, size_t len) {
  if (s < 0)
    s = 0;
  int m = s / 60;
  int sec = s % 60;
  snprintf(buf, len, "%d:%02d", m, sec);
}

static void play_event_cb(lv_event_t *e) {
  (void)e;
  is_playing = !is_playing;
  if (lbl_play_sym)
    lv_label_set_text(lbl_play_sym,
                      is_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
}

static void full_event_cb(lv_event_t *e) {
  (void)e;
  // Placeholder: toggle fullscreen could be implemented by user
}

void ui_video_init(void) {
  if (scr_video)
    return;

  scr_video = lv_obj_create(NULL);
  lv_obj_set_size(scr_video, 800, 480);
  lv_obj_remove_style_all(scr_video);

  // Top: video display area 800x380
  video_area = lv_obj_create(scr_video);
  lv_obj_set_size(video_area, 800, 380);
  lv_obj_set_pos(video_area, 0, 0);
  lv_obj_set_style_bg_color(video_area, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(video_area, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(video_area, 0, 0);
  lv_obj_set_style_radius(video_area, 0, 0);

  // Bottom: control bar 800x80
  ctrl_bar = lv_obj_create(scr_video);
  lv_obj_set_size(ctrl_bar, 800, 100);
  lv_obj_set_pos(ctrl_bar, 0, 380);
  lv_obj_set_style_bg_color(ctrl_bar, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(ctrl_bar, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(ctrl_bar, 0, 0);
  lv_obj_set_style_radius(ctrl_bar, 0, 0);

  // Layout: progress/time on left, buttons on right
  int btn_w = 40;
  int btn_h = 40;
  int gap = 12;
  int start_x = 560;

  // Progress area on the left: current time, slider, total time
  lbl_time_cur = lv_label_create(ctrl_bar);
  lv_label_set_text(lbl_time_cur, "0:00");
  lv_obj_set_style_text_color(lbl_time_cur, lv_color_make(0x33, 0x38, 0x3A), 0);
  lv_obj_set_pos(lbl_time_cur, 15, 28);

  slider = lv_slider_create(ctrl_bar);
  lv_obj_set_size(slider, 400, 8);
  lv_obj_set_pos(slider, 60, 34);
  lv_slider_set_range(slider, 0, 1000);
  lv_obj_add_event_cb(slider, NULL, LV_EVENT_VALUE_CHANGED, NULL);

  lbl_time_total = lv_label_create(ctrl_bar);
  lv_label_set_text(lbl_time_total, "0:00");
  lv_obj_set_style_text_color(lbl_time_total, lv_color_make(0x99, 0x9A, 0x9B),
                              0);
  lv_obj_set_pos(lbl_time_total, 470, 28);

  // Right control area: Prev, Play/Pause, Next, Fullscreen
  btn_prev = lv_btn_create(ctrl_bar);
  lv_obj_set_size(btn_prev, btn_w, btn_h);
  lv_obj_set_pos(btn_prev, start_x, 12);
  lv_obj_add_event_cb(btn_prev, NULL, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_prev = lv_label_create(btn_prev);
  lv_label_set_text(lbl_prev, LV_SYMBOL_PREV);

  btn_play = lv_btn_create(ctrl_bar);
  lv_obj_set_size(btn_play, btn_w, btn_h);
  lv_obj_set_pos(btn_play, start_x + (btn_w + gap) * 1, 12);
  lv_obj_add_event_cb(btn_play, play_event_cb, LV_EVENT_CLICKED, NULL);
  lbl_play_sym = lv_label_create(btn_play);
  lv_label_set_text(lbl_play_sym, LV_SYMBOL_PLAY);

  btn_next = lv_btn_create(ctrl_bar);
  lv_obj_set_size(btn_next, btn_w, btn_h);
  lv_obj_set_pos(btn_next, start_x + (btn_w + gap) * 2, 12);
  lv_obj_add_event_cb(btn_next, NULL, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_next = lv_label_create(btn_next);
  lv_label_set_text(lbl_next, LV_SYMBOL_NEXT);

  btn_full = lv_btn_create(ctrl_bar);
  lv_obj_set_size(btn_full, btn_w, btn_h);
  lv_obj_set_pos(btn_full, start_x + (btn_w + gap) * 3, 12);
  lv_obj_add_event_cb(btn_full, full_event_cb, LV_EVENT_CLICKED, NULL);
  lv_label_set_text(lv_label_create(btn_full), "⤢");

  /* bind swipe detection so children still receive clicks */
  lv_obj_add_event_cb(scr_video, video_overlay_event, LV_EVENT_ALL, NULL);
}

static void video_overlay_event(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_GESTURE) {
    lv_indev_t *ind = lv_indev_get_act();
    if (!ind)
      return;
    lv_dir_t dir = lv_indev_get_gesture_dir(ind);
    if (dir == LV_DIR_RIGHT) {
      ui_video_hide();
    }
    return;
  }

  if (code == LV_EVENT_PRESSED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    if (ind)
      lv_indev_get_point(ind, &p);
    touch_start_x_video = p.x;
  } else if (code == LV_EVENT_RELEASED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    if (ind)
      lv_indev_get_point(ind, &p);
    if (p.x - touch_start_x_video > 180) { /* right swipe threshold */
      ui_video_hide();
    }
  }
}

void ui_video_set_playing(bool playing) {
  is_playing = playing;
  if (lbl_play_sym)
    lv_label_set_text(lbl_play_sym,
                      is_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
}

void ui_video_set_time(int elapsed_seconds, int total_seconds) {
  cur_seconds = elapsed_seconds;
  tot_seconds = total_seconds;
  char buf[16];
  format_time(cur_seconds, buf, sizeof(buf));
  lv_label_set_text(lbl_time_cur, buf);
  format_time(tot_seconds, buf, sizeof(buf));
  lv_label_set_text(lbl_time_total, buf);
  if (tot_seconds > 0) {
    int val = (int)((int64_t)cur_seconds * 1000 / tot_seconds);
    if (val < 0)
      val = 0;
    if (val > 1000)
      val = 1000;
    lv_slider_set_value(slider, val, LV_ANIM_OFF);
  } else {
    lv_slider_set_value(slider, 0, LV_ANIM_OFF);
  }
}

void ui_video_show(void) {
  if (!scr_video)
    ui_video_init();
  scr_prev = lv_scr_act();
  lv_scr_load_anim(scr_video, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
}

void ui_video_hide(void) {
  if (scr_prev) {
    lv_scr_load_anim(scr_prev, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
    scr_prev = NULL;
  }
}
