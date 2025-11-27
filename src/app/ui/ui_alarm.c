// src/app/ui/ui_alarm.c
#include "app/ui_alarm.h"
#include "app/alarm.h"
#include "fonts.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static lv_obj_t *scr_alarm = NULL;
static lv_obj_t *list = NULL;
static lv_obj_t *btn_add = NULL; /* top-right add button */
static lv_obj_t *scr_prev = NULL;
static lv_coord_t touch_start_x_alarm = 0;
static lv_obj_t *hdr = NULL;
static lv_obj_t *lbl_countdown = NULL;
static lv_obj_t *lbl_title = NULL;

/* forward declarations */
static void card_click_cb(lv_event_t *e);
static void sw_event_cb(lv_event_t *e);
static void add_card_cb(lv_event_t *e);

/* Edit dialog state */
static lv_obj_t *dlg_overlay = NULL;
static lv_obj_t *dlg = NULL;
static alarm_t dlg_alarm;
static bool dlg_is_add = false;

static void alarm_overlay_event(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  /* If an edit/add dialog is open, ignore swipe/back gestures */
  if (dlg)
    return;
  if (code == LV_EVENT_GESTURE) {
    lv_indev_t *ind = lv_indev_get_act();
    if (!ind)
      return;
    lv_dir_t dir = lv_indev_get_gesture_dir(ind);
    if (dir == LV_DIR_LEFT) {
      ui_alarm_hide();
    }
    return;
  }

  /* Fallback: use press/release delta if gesture not available */
  if (code == LV_EVENT_PRESSED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    if (ind)
      lv_indev_get_point(ind, &p);
    touch_start_x_alarm = p.x;
  } else if (code == LV_EVENT_RELEASED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    if (ind)
      lv_indev_get_point(ind, &p);
    if (touch_start_x_alarm - p.x > 80) { /* left swipe */
      ui_alarm_hide();
    }
  }
}

static void item_event_cb(lv_event_t *e) {
  lv_obj_t *btn = lv_event_get_target(e);
  void *ud = lv_event_get_user_data(e);
  size_t idx = (size_t)ud;
  // toggle enable state
  alarm_t *arr = NULL;
  size_t n = 0;
  if (!alarm_list(&arr, &n))
    return;
  if (idx < n) {
    alarm_enable(arr[idx].id, !arr[idx].enabled);
  }
  free(arr);
  ui_alarm_refresh();
}

/* Close dialog helper */
static void close_dialog(void) {
  if (dlg) {
    lv_obj_del(dlg);
    dlg = NULL;
  }
  if (dlg_overlay) {
    lv_obj_del(dlg_overlay);
    dlg_overlay = NULL;
  }
  ui_alarm_refresh();
}

/* Dialog cancel callback */
static void dlg_cancel_cb(lv_event_t *e) {
  (void)e;
  close_dialog();
}

/* Dialog save callback */
static void dlg_save_cb(lv_event_t *e) {
  (void)e;
  if (dlg_is_add) {
    alarm_add(&dlg_alarm);
  } else {
    alarm_update(dlg_alarm.id, &dlg_alarm);
  }
  alarm_save_now();
  close_dialog();
}

/* Create and show edit/add dialog. If a is NULL, opens add dialog with defaults
 */
static void show_edit_dialog(const alarm_t *a, bool is_add) {
  if (dlg)
    return; /* already open */
  dlg_is_add = is_add;
  if (a)
    dlg_alarm = *a;
  else {
    memset(&dlg_alarm, 0, sizeof(dlg_alarm));
    time_t now = time(NULL);
    snprintf(dlg_alarm.id, sizeof(dlg_alarm.id), "a%ld", now);
    dlg_alarm.hour = 7;
    dlg_alarm.minute = 0;
    dlg_alarm.enabled = true;
  }

  /* overlay to capture outside clicks */
  dlg_overlay = lv_obj_create(scr_alarm);
  lv_obj_set_size(dlg_overlay, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_opa(dlg_overlay, LV_OPA_50, 0);
  lv_obj_add_event_cb(dlg_overlay, dlg_cancel_cb, LV_EVENT_CLICKED, NULL);

  dlg = lv_obj_create(scr_alarm);
  lv_obj_set_size(dlg, LV_PCT(80), LV_PCT(60));
  lv_obj_center(dlg);

  lv_obj_t *title = lv_label_create(dlg);
  lv_label_set_text(title, is_add ? "添加闹钟" : "编辑闹钟");
  lv_obj_set_style_text_font(title, &LXGWWenKaiMono_Light_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

  /* Time display (simple) */
  char timebuf[16];
  snprintf(timebuf, sizeof(timebuf), "%02d:%02d", dlg_alarm.hour,
           dlg_alarm.minute);
  lv_obj_t *lbl_time = lv_label_create(dlg);
  lv_label_set_text(lbl_time, timebuf);
  lv_obj_set_style_text_font(lbl_time, &LXGWWenKaiMono_Light_24, 0);
  lv_obj_align(lbl_time, LV_ALIGN_CENTER, -10, -10);

  /* Buttons */
  lv_obj_t *btn_save = lv_btn_create(dlg);
  lv_obj_set_size(btn_save, 80, 36);
  lv_obj_align(btn_save, LV_ALIGN_BOTTOM_RIGHT, -12, -12);
  lv_obj_t *lbls = lv_label_create(btn_save);
  lv_label_set_text(lbls, "完成");
  lv_obj_set_style_text_font(lbls, &LXGWWenKaiMono_Light_14, 0);
  lv_obj_add_event_cb(btn_save, dlg_save_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *btn_cancel = lv_btn_create(dlg);
  lv_obj_set_size(btn_cancel, 80, 36);
  lv_obj_align(btn_cancel, LV_ALIGN_BOTTOM_LEFT, 12, -12);
  lv_obj_t *lblc = lv_label_create(btn_cancel);
  lv_label_set_text(lblc, "取消");
  lv_obj_set_style_text_font(lblc, &LXGWWenKaiMono_Light_14, 0);
  lv_obj_add_event_cb(btn_cancel, dlg_cancel_cb, LV_EVENT_CLICKED, NULL);
}

/* Back button removed; use left-swipe to return to previous screen */

static void add_event_cb(lv_event_t *e) {
  (void)e;
  time_t now = time(NULL);
  struct tm tm;
  localtime_r(&now, &tm);
  alarm_t a;
  memset(&a, 0, sizeof(a));
  snprintf(a.id, sizeof(a.id), "a%ld", now);
  snprintf(a.label, sizeof(a.label), "quick");
  a.enabled = true;
  tm.tm_min += 1;
  mktime(&tm);
  a.hour = tm.tm_hour;
  a.minute = tm.tm_min;
  a.remove_after_trigger = false;
  alarm_add(&a);
  ui_alarm_refresh();
}

void ui_alarm_init(void) {
  if (scr_alarm)
    return;
  scr_alarm = lv_obj_create(NULL);
  /* header: title + countdown */
  hdr = lv_obj_create(scr_alarm);
  lv_obj_set_size(hdr, LV_PCT(100), 56);
  lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_pad_all(hdr, 8, 0);
  lv_obj_set_style_bg_color(hdr, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);

  lbl_title = lv_label_create(hdr);
  lv_label_set_text(lbl_title, "闹钟");
  lv_obj_set_style_text_font(lbl_title, &PingFangSC_Regular_28, 0);
  lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x1C1C1E), 0);
  lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 14, 6);

  /* top-right add button '+' */
  btn_add = lv_label_create(hdr);
  lv_label_set_text(btn_add, "+");
  lv_obj_set_style_text_font(btn_add, &PingFangSC_Semibold_40, 0);
  lv_obj_set_style_text_color(btn_add, lv_color_hex(0x007AFF), 0);
  lv_obj_align(btn_add, LV_ALIGN_RIGHT_MID, -12, 4);
  lv_obj_add_event_cb(btn_add, add_card_cb, LV_EVENT_CLICKED, NULL);

  /* Move countdown out of header into a content area below header (iOS style)
   */
  lv_obj_t *cnt_area = lv_obj_create(scr_alarm);
  lv_obj_set_size(cnt_area, LV_PCT(100), 64);
  lv_obj_align(cnt_area, LV_ALIGN_TOP_MID, 0, 56);
  lv_obj_set_style_bg_color(cnt_area, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_bg_opa(cnt_area, LV_OPA_TRANSP, 0);

  lbl_countdown = lv_label_create(cnt_area);
  lv_label_set_text(lbl_countdown, "");
  lv_obj_set_style_text_font(lbl_countdown, &PingFangSC_Regular_24, 0);
  lv_obj_set_style_text_color(lbl_countdown, lv_color_hex(0x1C1C1E), 0);
  lv_obj_align(lbl_countdown, LV_ALIGN_CENTER, 0, 8);

  list = lv_list_create(scr_alarm);
  lv_obj_set_size(list, LV_PCT(100), LV_PCT(72));
  lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 132);

  /* bind swipe detection to the screen object so children still receive clicks
   */
  lv_obj_add_event_cb(scr_alarm, alarm_overlay_event, LV_EVENT_ALL, NULL);

  /* top Add button removed — use the final card as add entry */
}

void ui_alarm_show(void) {
  if (!scr_alarm)
    ui_alarm_init();
  scr_prev = lv_scr_act();
  lv_scr_load(scr_alarm);
  ui_alarm_refresh();
}

void ui_alarm_hide(void) {
  if (scr_prev) {
    lv_scr_load(scr_prev);
    scr_prev = NULL;
  }
}

void ui_alarm_refresh(void) {
  if (!list)
    return;
  lv_obj_clean(list);
  alarm_t *arr = NULL;
  size_t n = 0;
  if (!alarm_list(&arr, &n))
    return;
  /* compute next alarm countdown */
  time_t now = time(NULL);
  time_t next = 0;
  for (size_t i = 0; i < n; ++i) {
    if (!arr[i].enabled)
      continue;
    /* build next occurrence today/tomorrow ignoring repeat for simplicity */
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    struct tm tm_target = tm_now;
    tm_target.tm_hour = arr[i].hour;
    tm_target.tm_min = arr[i].minute;
    tm_target.tm_sec = 0;
    time_t t = mktime(&tm_target);
    if (t <= now)
      t += 24 * 3600; /* next day */
    if (next == 0 || t < next)
      next = t;
  }
  if (lbl_countdown) {
    if (next == 0) {
      lv_label_set_text(lbl_countdown, "无计划闹钟");
    } else {
      int diff = (int)difftime(next, now);
      int h = diff / 3600;
      int m = (diff % 3600) / 60;
      char buf[64];
      snprintf(buf, sizeof(buf), "%d小时%02d分钟后响铃", h, m);
      lv_label_set_text(lbl_countdown, buf);
    }
  }
  char buf[128];
  for (size_t i = 0; i < n; ++i) {
    /* Render each alarm as a card-like container */
    lv_obj_t *card = lv_obj_create(list);
    lv_obj_set_size(card, lv_pct(94), 66);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_10, 0);
    lv_obj_set_style_shadow_width(card, 6, 0);

    /* small period label */
    lv_obj_t *lbl_period = lv_label_create(card);
    lv_label_set_text(lbl_period, arr[i].hour < 12 ? "上午" : "下午");
    lv_obj_set_style_text_font(lbl_period, &PingFangSC_Regular_14, 0);
    lv_obj_set_style_text_color(lbl_period, lv_color_hex(0x8E8E93), 0);
    lv_obj_align(lbl_period, LV_ALIGN_LEFT_MID, 14, -14);

    /* big time */
    char tbuf[16];
    snprintf(tbuf, sizeof(tbuf), "%02d:%02d", arr[i].hour, arr[i].minute);
    lv_obj_t *lbl_time = lv_label_create(card);
    lv_label_set_text(lbl_time, tbuf);
    lv_obj_set_style_text_font(lbl_time, &PingFangSC_Semibold_38, 0);
    lv_obj_set_style_text_color(lbl_time, lv_color_hex(0x1C1C1E), 0);
    lv_obj_align(lbl_time, LV_ALIGN_LEFT_MID, 56, -4);

    /* repeat info */
    lv_obj_t *lbl_repeat = lv_label_create(card);
    /* build repeat string */
    char rbuf[64] = {0};
    if (arr[i].repeat[0] && arr[i].repeat[1] && arr[i].repeat[2] &&
        arr[i].repeat[3] && arr[i].repeat[4] && arr[i].repeat[5] &&
        arr[i].repeat[6])
      snprintf(rbuf, sizeof(rbuf), "每天");
    else if (!arr[i].repeat[0] && !arr[i].repeat[1] && !arr[i].repeat[2] &&
             !arr[i].repeat[3] && !arr[i].repeat[4] && !arr[i].repeat[5] &&
             !arr[i].repeat[6])
      snprintf(rbuf, sizeof(rbuf), "仅一次");
    else {
      const char *names[] = {"周日", "周一", "周二", "周三",
                             "周四", "周五", "周六"};
      int pos = 0;
      for (int d = 0; d < 7; ++d) {
        if (arr[i].repeat[d]) {
          if (pos)
            strncat(rbuf, " ", sizeof(rbuf) - strlen(rbuf) - 1);
          strncat(rbuf, names[d], sizeof(rbuf) - strlen(rbuf) - 1);
          pos++;
        }
      }
    }
    lv_label_set_text(lbl_repeat, rbuf);
    lv_obj_set_style_text_font(lbl_repeat, &PingFangSC_Regular_18, 0);
    lv_obj_set_style_text_color(lbl_repeat, lv_color_hex(0x8E8E93), 0);
    lv_obj_align(lbl_repeat, LV_ALIGN_LEFT_MID, 56, 18);

    /* capsule switch */
    lv_obj_t *sw = lv_switch_create(card);
    if (arr[i].enabled)
      lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -18, 0);
    /* style switch thumb and indicator */
    lv_obj_set_style_bg_color(sw, lv_color_hex(0xE6E9F2), 0);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sw, 0, 0);
    /* switch event toggles enable */
    lv_obj_add_event_cb(sw, sw_event_cb, LV_EVENT_VALUE_CHANGED,
                        (void *)(size_t)i);

    /* clicking card opens edit dialog */
    lv_obj_add_event_cb(card, card_click_cb, LV_EVENT_CLICKED,
                        (void *)(size_t)i);
    /* also listen for swipe gestures on the card itself */
    lv_obj_add_event_cb(card, alarm_overlay_event, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
  }

  /* add final card as Add entry */
  lv_obj_t *add_card = lv_obj_create(list);
  lv_obj_set_size(add_card, lv_pct(94), 66);
  lv_obj_set_style_pad_all(add_card, 10, 0);
  lv_obj_set_style_radius(add_card, 14, 0);
  lv_obj_set_style_bg_color(add_card, lv_color_hex(0x0A84FF), 0);
  lv_obj_set_style_bg_opa(add_card, LV_OPA_COVER, 0);
  lv_obj_set_style_shadow_color(add_card, lv_color_hex(0x0A62D6), 0);
  lv_obj_set_style_shadow_opa(add_card, LV_OPA_30, 0);
  lv_obj_set_style_shadow_width(add_card, 8, 0);
  lv_obj_t *lbl_plus = lv_label_create(add_card);
  lv_label_set_text(lbl_plus, "添加闹钟");
  lv_obj_set_style_text_font(lbl_plus, &PingFangSC_Regular_18, 0);
  lv_obj_set_style_text_color(lbl_plus, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(lbl_plus, LV_ALIGN_CENTER, 0, 0);
  /* bind add card click to open add dialog */
  lv_obj_add_event_cb(add_card, add_card_cb, LV_EVENT_CLICKED, NULL);
  /* listen for swipe on add card too */
  lv_obj_add_event_cb(add_card, alarm_overlay_event, LV_EVENT_ALL, NULL);
}

/* Card click callback: fetch alarm array and open edit dialog */
static void card_click_cb(lv_event_t *e) {
  void *ud = lv_event_get_user_data(e);
  size_t idx = (size_t)ud;
  alarm_t *arr = NULL;
  size_t n = 0;
  if (!alarm_list(&arr, &n))
    return;
  if (idx < n)
    show_edit_dialog(&arr[idx], false);
  free(arr);
}

static void sw_event_cb(lv_event_t *e) {
  void *ud = lv_event_get_user_data(e);
  size_t idx = (size_t)ud;
  alarm_t *arr = NULL;
  size_t n = 0;
  if (!alarm_list(&arr, &n))
    return;
  if (idx < n) {
    alarm_enable(arr[idx].id, !arr[idx].enabled);
  }
  free(arr);
  ui_alarm_refresh();
}

/* add card click callback */
static void add_card_cb(lv_event_t *e) {
  (void)e;
  show_edit_dialog(NULL, true);
}
