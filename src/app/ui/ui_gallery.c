// src/app/ui/ui_gallery.c

#include "app/ui/ui_gallery.h"
#include "fonts.h"
#include "lvgl.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Gallery screen state */
static lv_obj_t *scr_gallery = NULL;
static lv_obj_t *scr_prev = NULL;
static lv_coord_t touch_start_y = 0;

/* Gallery content containers */
static lv_obj_t *img_container = NULL;
static lv_obj_t *lbl_title = NULL;
static lv_obj_t *lbl_info = NULL;

/* Image gallery state */
static char **image_paths = NULL;
static int image_count = 0;
static int current_index = 0;

/* Forward declarations */
static void gallery_event_cb(lv_event_t *e);
static void scan_images(const char *dir);
static void display_current_image(void);
static void free_image_list(void);
static void btn_prev_cb(lv_event_t *e);
static void btn_next_cb(lv_event_t *e);

/**
 * @brief Main event handler for gallery screen (handles down-swipe to exit)
 */
static void gallery_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_GESTURE) {
    lv_indev_t *ind = lv_indev_get_act();
    if (!ind)
      return;
    lv_dir_t dir = lv_indev_get_gesture_dir(ind);
    if (dir == LV_DIR_BOTTOM) {
      /* Down swipe: exit gallery */
      ui_gallery_hide();
    }
    return;
  }

  if (code == LV_EVENT_PRESSED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    if (ind)
      lv_indev_get_point(ind, &p);
    touch_start_y = p.y;
  } else if (code == LV_EVENT_RELEASED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    if (ind)
      lv_indev_get_point(ind, &p);
    /* Down swipe: exit */
    if (p.y - touch_start_y > 80) {
      ui_gallery_hide();
    }
  }
}

/**
 * @brief Previous image button callback
 */
static void btn_prev_cb(lv_event_t *e) {
  (void)e;
  if (image_count == 0)
    return;
  current_index--;
  if (current_index < 0)
    current_index = image_count - 1;
  display_current_image();
}

/**
 * @brief Next image button callback
 */
static void btn_next_cb(lv_event_t *e) {
  (void)e;
  if (image_count == 0)
    return;
  current_index++;
  if (current_index >= image_count)
    current_index = 0;
  display_current_image();
}

/**
 * @brief Scan directory for image files
 */
static void scan_images(const char *dir) {
  free_image_list();

  if (!dir)
    return;

  DIR *d = opendir(dir);
  if (!d) {
    printf("[Gallery] Failed to open directory: %s\n", dir);
    return;
  }

  struct dirent *ent;
  while ((ent = readdir(d)) != NULL) {
    if (ent->d_type == DT_REG) {
      const char *name = ent->d_name;
      const char *ext = strrchr(name, '.');
      if (!ext)
        continue;

      /* Filter for common image extensions */
      if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0 ||
          strcasecmp(ext, ".png") == 0 || strcasecmp(ext, ".bmp") == 0) {
        image_paths = realloc(image_paths, sizeof(char *) * (image_count + 1));
        char *full = malloc(strlen(dir) + 1 + strlen(name) + 1);
        sprintf(full, "%s/%s", dir, name);
        image_paths[image_count++] = full;
      }
    }
  }
  closedir(d);

  printf("[Gallery] Found %d images in %s\n", image_count, dir);
}

/**
 * @brief Display the current image in the gallery
 */
static void display_current_image(void) {
  if (!img_container)
    return;

  /* Clear previous content */
  lv_obj_clean(img_container);

  if (image_count == 0) {
    /* Show placeholder text */
    lv_obj_t *lbl_empty = lv_label_create(img_container);
    lv_label_set_text(lbl_empty, "无可用图片\n请将图片放入指定目录");
    lv_obj_set_style_text_font(lbl_empty, &PingFangSC_Regular_24, 0);
    lv_obj_set_style_text_color(lbl_empty, lv_color_hex(0x8E8E93), 0);
    lv_obj_set_style_text_align(lbl_empty, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lbl_empty);

    if (lbl_title)
      lv_label_set_text(lbl_title, "电子相册");
    if (lbl_info)
      lv_label_set_text(lbl_info, "");
    return;
  }

  /* Create image object */
  lv_obj_t *img = lv_img_create(img_container);
  /* TODO: Load actual image file */
  /* lv_img_set_src(img, image_paths[current_index]); */

  /* For now, show a placeholder */
  lv_obj_set_size(img, 400, 300);
  lv_obj_set_style_bg_color(img, lv_color_hex(0x3C3C3E), 0);
  lv_obj_set_style_bg_opa(img, LV_OPA_COVER, 0);
  lv_obj_center(img);

  /* Update title and info */
  if (lbl_title) {
    char title_buf[128];
    const char *filename = strrchr(image_paths[current_index], '/');
    if (filename)
      filename++;
    else
      filename = image_paths[current_index];
    snprintf(title_buf, sizeof(title_buf), "%.80s", filename);
    lv_label_set_text(lbl_title, title_buf);
  }

  if (lbl_info) {
    char info_buf[64];
    snprintf(info_buf, sizeof(info_buf), "%d / %d", current_index + 1,
             image_count);
    lv_label_set_text(lbl_info, info_buf);
  }
}

/**
 * @brief Free image list memory
 */
static void free_image_list(void) {
  if (!image_paths)
    return;
  for (int i = 0; i < image_count; ++i)
    free(image_paths[i]);
  free(image_paths);
  image_paths = NULL;
  image_count = 0;
  current_index = 0;
}

/**
 * @brief Initialize gallery UI (called once)
 */
void ui_gallery_init(void) {
  if (scr_gallery)
    return;

  scr_gallery = lv_obj_create(NULL);

  /* Set dark background (iOS-like dark mode) */
  lv_obj_set_style_bg_color(scr_gallery, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(scr_gallery, LV_OPA_COVER, 0);

  /* Header with title */
  lv_obj_t *hdr = lv_obj_create(scr_gallery);
  lv_obj_set_size(hdr, LV_PCT(100), 60);
  lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(hdr, lv_color_hex(0x1C1C1E), 0);
  lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(hdr, 0, 0);
  lv_obj_set_style_radius(hdr, 0, 0);

  lbl_title = lv_label_create(hdr);
  lv_label_set_text(lbl_title, "电子相册");
  lv_obj_set_style_text_font(lbl_title, &PingFangSC_Regular_28, 0);
  lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_center(lbl_title);

  /* Main image display area */
  img_container = lv_obj_create(scr_gallery);
  lv_obj_set_size(img_container, LV_PCT(100), LV_PCT(70));
  lv_obj_align(img_container, LV_ALIGN_CENTER, 0, 10);
  lv_obj_set_style_bg_color(img_container, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(img_container, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(img_container, 0, 0);
  lv_obj_clear_flag(img_container, LV_OBJ_FLAG_SCROLLABLE);

  /* Bottom control bar */
  lv_obj_t *ctrl_bar = lv_obj_create(scr_gallery);
  lv_obj_set_size(ctrl_bar, LV_PCT(100), 80);
  lv_obj_align(ctrl_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(ctrl_bar, lv_color_hex(0x1C1C1E), 0);
  lv_obj_set_style_bg_opa(ctrl_bar, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(ctrl_bar, 0, 0);
  lv_obj_set_style_radius(ctrl_bar, 0, 0);

  /* Info label (image counter) */
  lbl_info = lv_label_create(ctrl_bar);
  lv_label_set_text(lbl_info, "0 / 0");
  lv_obj_set_style_text_font(lbl_info, &PingFangSC_Regular_18, 0);
  lv_obj_set_style_text_color(lbl_info, lv_color_hex(0x8E8E93), 0);
  lv_obj_align(lbl_info, LV_ALIGN_TOP_MID, 0, 8);

  /* Previous button */
  lv_obj_t *btn_prev = lv_btn_create(ctrl_bar);
  lv_obj_set_size(btn_prev, 100, 40);
  lv_obj_align(btn_prev, LV_ALIGN_BOTTOM_LEFT, 40, -12);
  lv_obj_t *lbl_prev = lv_label_create(btn_prev);
  lv_label_set_text(lbl_prev, "上一张");
  lv_obj_set_style_text_font(lbl_prev, &PingFangSC_Regular_18, 0);
  lv_obj_add_event_cb(btn_prev, btn_prev_cb, LV_EVENT_CLICKED, NULL);

  /* Next button */
  lv_obj_t *btn_next = lv_btn_create(ctrl_bar);
  lv_obj_set_size(btn_next, 100, 40);
  lv_obj_align(btn_next, LV_ALIGN_BOTTOM_RIGHT, -40, -12);
  lv_obj_t *lbl_next = lv_label_create(btn_next);
  lv_label_set_text(lbl_next, "下一张");
  lv_obj_set_style_text_font(lbl_next, &PingFangSC_Regular_18, 0);
  lv_obj_add_event_cb(btn_next, btn_next_cb, LV_EVENT_CLICKED, NULL);

  /* Bind swipe detection to screen */
  lv_obj_add_event_cb(scr_gallery, gallery_event_cb, LV_EVENT_ALL, NULL);

  printf("[Gallery] UI initialized\n");
}

/**
 * @brief Show gallery screen
 */
void ui_gallery_show(void) {
  if (!scr_gallery)
    ui_gallery_init();

  scr_prev = lv_scr_act();
  lv_scr_load(scr_gallery);

  /* Scan for images (you can change the path) */
  /* TODO: Make this path configurable */
  scan_images("/root/gallery");
  current_index = 0;
  display_current_image();

  printf("[Gallery] Screen shown\n");
}

/**
 * @brief Hide gallery and return to previous screen
 */
void ui_gallery_hide(void) {
  if (scr_prev) {
    lv_scr_load(scr_prev);
    scr_prev = NULL;
  }
  printf("[Gallery] Screen hidden\n");
}

/**
 * @brief Refresh gallery content
 */
void ui_gallery_refresh(void) {
  scan_images("/root/gallery");
  current_index = 0;
  display_current_image();
}
