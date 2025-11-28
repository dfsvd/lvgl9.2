// src/app/ui/ui_secondary.c

#include "app/ui/ui_secondary.h"
#include "lvgl.h"

static lv_obj_t *scr_secondary = NULL;
static lv_obj_t *scr_prev = NULL;
static lv_coord_t touch_start_x = 0;

static void secondary_overlay_event(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    if (ind)
      lv_indev_get_point(ind, &p);
    touch_start_x = p.x;
  } else if (code == LV_EVENT_RELEASED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    if (ind)
      lv_indev_get_point(ind, &p);
    if (p.x - touch_start_x > 80) { // right swipe to go back
      ui_secondary_hide();
    }
  }
}

void ui_secondary_init(void) {
  if (scr_secondary)
    return;
  scr_secondary = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_secondary, lv_color_hex(0x111111), 0);
  lv_obj_set_style_bg_opa(scr_secondary, LV_OPA_COVER, 0);

  lv_obj_t *lbl = lv_label_create(scr_secondary);
  lv_label_set_text(lbl, "二级界面\n左滑返回");
  lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
  lv_obj_center(lbl);

  // overlay to capture swipes
  lv_obj_add_event_cb(scr_secondary, secondary_overlay_event, LV_EVENT_ALL,
                      NULL);
}

void ui_secondary_show(void) {
  if (!scr_secondary)
    ui_secondary_init();
  scr_prev = lv_scr_act();
  lv_scr_load(scr_secondary);
}

void ui_secondary_hide(void) {
  if (scr_prev) {
    lv_scr_load(scr_prev);
    scr_prev = NULL;
  }
}
