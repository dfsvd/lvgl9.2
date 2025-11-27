// src/app/ui/ui_alarm.c
#include "app/ui_alarm.h"
#include "app/alarm.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static lv_obj_t *scr_alarm = NULL;
static lv_obj_t *list = NULL;
static lv_obj_t *btn_back = NULL;
static lv_obj_t *btn_add = NULL;

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

static void back_event_cb(lv_event_t *e) {
  (void)e;
  ui_alarm_hide();
}

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
  list = lv_list_create(scr_alarm);
  lv_obj_set_size(list, LV_PCT(100), LV_PCT(80));
  lv_obj_align(list, LV_ALIGN_CENTER, 0, 20);

  btn_back = lv_btn_create(scr_alarm);
  lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 4, 4);
  lv_obj_t *lbl = lv_label_create(btn_back);
  lv_label_set_text(lbl, "Back");
  lv_obj_add_event_cb(btn_back, back_event_cb, LV_EVENT_CLICKED, NULL);

  btn_add = lv_btn_create(scr_alarm);
  lv_obj_align(btn_add, LV_ALIGN_TOP_RIGHT, -4, 4);
  lv_obj_t *lbl2 = lv_label_create(btn_add);
  lv_label_set_text(lbl2, "Add");
  lv_obj_add_event_cb(btn_add, add_event_cb, LV_EVENT_CLICKED, NULL);
}

void ui_alarm_show(void) {
  if (!scr_alarm)
    ui_alarm_init();
  lv_scr_load(scr_alarm);
  ui_alarm_refresh();
}

void ui_alarm_hide(void) {
  lv_obj_t *prev = lv_scr_act();
  // naive: load the default screen back
  lv_scr_load(lv_obj_get_parent(prev));
}

void ui_alarm_refresh(void) {
  if (!list)
    return;
  lv_obj_clean(list);
  alarm_t *arr = NULL;
  size_t n = 0;
  if (!alarm_list(&arr, &n))
    return;
  char buf[128];
  for (size_t i = 0; i < n; ++i) {
    snprintf(buf, sizeof(buf), "%02d:%02d %s %s", arr[i].hour, arr[i].minute,
             arr[i].label, arr[i].enabled ? "(on)" : "(off)");
    lv_obj_t *btn = lv_list_add_button(list, NULL, buf);
    lv_obj_add_event_cb(btn, item_event_cb, LV_EVENT_CLICKED,
                        (void *)(size_t)i);
  }
  free(arr);
}
