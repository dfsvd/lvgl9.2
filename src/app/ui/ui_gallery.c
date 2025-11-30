// src/app/ui/ui_gallery.c

#include "app/ui/ui_gallery.h"
#include "fonts.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Declare the compiled image resources */
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

/* Gallery screen state */
static lv_obj_t *scr_gallery = NULL;
static lv_obj_t *scr_prev = NULL;
static lv_coord_t touch_start_y = 0;
static lv_coord_t touch_start_x = 0;

/* Gallery content containers */
static lv_obj_t *img_container = NULL;
static lv_obj_t *lbl_title = NULL;
static lv_obj_t *lbl_info = NULL;
static lv_obj_t *btn_play_pause = NULL;
static lv_obj_t *lbl_play_pause = NULL;
static lv_obj_t *hdr = NULL;
static lv_obj_t *ctrl_bar = NULL;

/* Fullscreen mode state */
static lv_obj_t *fullscreen_container = NULL;
static bool is_fullscreen = false;

/* Slideshow state */
static lv_timer_t *slideshow_timer = NULL;
static bool is_playing = false;
static const uint32_t SLIDESHOW_INTERVAL = 3000; /* 3 seconds per image */

/* Image gallery state - using compiled image resources */
static const lv_image_dsc_t *image_list[] = {
    &image_1, &image_2, &image_3, &image_4, &image_5,
    &image_6, &image_7, &image_8, &image_9, &image_10};
static const int image_count = sizeof(image_list) / sizeof(image_list[0]);
static int current_index = 0;

/* Forward declarations */
static void gallery_event_cb(lv_event_t *e);
static void display_current_image(void);
static void animation_update_cb(lv_timer_t *t);
static void display_image_with_animation(int new_index, lv_anim_path_cb_t path);
static void btn_prev_cb(lv_event_t *e);
static void btn_next_cb(lv_event_t *e);
static void btn_play_pause_cb(lv_event_t *e);
static void slideshow_timer_cb(lv_timer_t *timer);
static void start_slideshow(void);
static void stop_slideshow(void);
static void save_gallery_state(void);
static void load_gallery_state(void);
static void img_swipe_event_cb(lv_event_t *e);
static void img_click_cb(lv_event_t *e);
static void enter_fullscreen(void);
static void exit_fullscreen(void);
static void fullscreen_event_cb(lv_event_t *e);
static void display_fullscreen_image(void);

/**
 * @brief Main event handler for gallery screen (handles down-swipe to exit)
 */
static void gallery_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);

  /* Skip gesture handling in fullscreen mode */
  if (is_fullscreen)
    return;

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
    touch_start_x = p.x;
  } else if (code == LV_EVENT_RELEASED) {
    /* Skip if in fullscreen mode */
    if (is_fullscreen)
      return;

    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    if (ind)
      lv_indev_get_point(ind, &p);

    int dx = p.x - touch_start_x;
    int dy = p.y - touch_start_y;

    /* Determine swipe direction */
    if (abs(dy) > abs(dx)) {
      /* Vertical swipe */
      if (dy > 80) {
        /* Down swipe: exit */
        ui_gallery_hide();
      }
    } else {
      /* Horizontal swipe */
      if (dx < -100) {
        /* Left swipe: next image */
        btn_next_cb(NULL);
      } else if (dx > 100) {
        /* Right swipe: previous image */
        btn_prev_cb(NULL);
      }
    }
  }
} /**
   * @brief Previous image button callback
   */
static void btn_prev_cb(lv_event_t *e) {
  (void)e;
  if (image_count == 0)
    return;
  int new_index = current_index - 1;
  if (new_index < 0)
    new_index = image_count - 1;
  display_image_with_animation(new_index, lv_anim_path_ease_in_out);
  save_gallery_state();
}

/**
 * @brief Next image button callback
 */
static void btn_next_cb(lv_event_t *e) {
  (void)e;
  if (image_count == 0)
    return;
  int new_index = current_index + 1;
  if (new_index >= image_count)
    new_index = 0;
  display_image_with_animation(new_index, lv_anim_path_ease_in_out);
  save_gallery_state();
}

/**
 * @brief Play/Pause button callback
 */
static void btn_play_pause_cb(lv_event_t *e) {
  (void)e;
  if (is_playing) {
    stop_slideshow();
  } else {
    start_slideshow();
  }
}

/**
 * @brief Display the current image in the gallery
 */
static void display_current_image(void) {
  if (!img_container)
    return;

  /* Clear previous content */
  lv_obj_clean(img_container);

  if (image_count == 0 || current_index < 0 || current_index >= image_count) {
    /* Show placeholder text */
    lv_obj_t *lbl_empty = lv_label_create(img_container);
    lv_label_set_text(lbl_empty, "无可用图片");
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

  /* Create image object and load from compiled resource */
  lv_obj_t *img = lv_img_create(img_container);
  lv_img_set_src(img, image_list[current_index]);
  lv_obj_center(img);

  /* Make image clickable to enter fullscreen */
  lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(img, img_click_cb, LV_EVENT_CLICKED, NULL);

  /* Update title and info */
  if (lbl_title) {
    char title_buf[64];
    snprintf(title_buf, sizeof(title_buf), "图片 %d", current_index + 1);
    lv_label_set_text(lbl_title, title_buf);
  }

  if (lbl_info) {
    char info_buf[64];
    snprintf(info_buf, sizeof(info_buf), "%d / %d", current_index + 1,
             image_count);
    lv_label_set_text(lbl_info, info_buf);
  }

  printf("[Gallery] Displaying image %d/%d\n", current_index + 1, image_count);
}

/**
 * @brief Display image with fade animation
 */
/* Helper function for animation update */
static void animation_update_cb(lv_timer_t *t) {
  display_current_image();

  /* Fade in animation */
  lv_anim_t anim_in;
  lv_anim_init(&anim_in);
  lv_anim_set_var(&anim_in, img_container);
  lv_anim_set_values(&anim_in, LV_OPA_TRANSP, LV_OPA_COVER);
  lv_anim_set_time(&anim_in, 200);
  lv_anim_set_path_cb(&anim_in, lv_anim_path_ease_in_out);
  lv_anim_set_exec_cb(&anim_in, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
  lv_anim_start(&anim_in);

  lv_timer_del(t);
}

static void display_image_with_animation(int new_index,
                                         lv_anim_path_cb_t path) {
  if (new_index < 0 || new_index >= image_count)
    return;

  current_index = new_index;

  /* Fade out animation */
  lv_anim_t anim_out;
  lv_anim_init(&anim_out);
  lv_anim_set_var(&anim_out, img_container);
  lv_anim_set_values(&anim_out, LV_OPA_COVER, LV_OPA_TRANSP);
  lv_anim_set_time(&anim_out, 200);
  lv_anim_set_path_cb(&anim_out, path);
  lv_anim_set_exec_cb(&anim_out, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
  lv_anim_start(&anim_out);

  /* Update content after fade out */
  lv_timer_t *update_timer = lv_timer_create(animation_update_cb, 200, NULL);
  lv_timer_set_repeat_count(update_timer, 1);
}

/**
 * @brief Slideshow timer callback
 */
static void slideshow_timer_cb(lv_timer_t *timer) {
  (void)timer;
  if (!is_playing)
    return;

  /* Advance to next image */
  int new_index = current_index + 1;
  if (new_index >= image_count)
    new_index = 0;
  current_index = new_index;

  /* Update display based on current mode */
  if (is_fullscreen) {
    display_fullscreen_image();
  } else {
    display_image_with_animation(current_index, lv_anim_path_ease_in_out);
  }

  save_gallery_state();
}

/**
 * @brief Start slideshow
 */
static void start_slideshow(void) {
  if (is_playing)
    return;

  is_playing = true;
  if (lbl_play_pause) {
    lv_label_set_text(lbl_play_pause, "暂停");
  }

  if (!slideshow_timer) {
    slideshow_timer =
        lv_timer_create(slideshow_timer_cb, SLIDESHOW_INTERVAL, NULL);
  } else {
    lv_timer_resume(slideshow_timer);
  }

  printf("[Gallery] Slideshow started\n");
}

/**
 * @brief Stop slideshow
 */
static void stop_slideshow(void) {
  if (!is_playing)
    return;

  is_playing = false;
  if (lbl_play_pause) {
    lv_label_set_text(lbl_play_pause, "播放");
  }

  if (slideshow_timer) {
    lv_timer_pause(slideshow_timer);
  }

  printf("[Gallery] Slideshow stopped\n");
}

/**
 * @brief Save gallery state to file
 */
static void save_gallery_state(void) {
  FILE *f = fopen("/tmp/gallery_state.txt", "w");
  if (f) {
    fprintf(f, "%d\n%d\n", current_index, is_playing ? 1 : 0);
    fclose(f);
  }
}

/**
 * @brief Load gallery state from file
 */
static void load_gallery_state(void) {
  FILE *f = fopen("/tmp/gallery_state.txt", "r");
  if (f) {
    int saved_index = 0;
    int saved_playing = 0;
    if (fscanf(f, "%d\n%d\n", &saved_index, &saved_playing) == 2) {
      if (saved_index >= 0 && saved_index < image_count) {
        current_index = saved_index;
      }
      if (saved_playing && !is_playing) {
        start_slideshow();
      }
    }
    fclose(f);
    printf("[Gallery] State loaded: index=%d, playing=%d\n", current_index,
           is_playing);
  }
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
  hdr = lv_obj_create(scr_gallery);
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
  ctrl_bar = lv_obj_create(scr_gallery);
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
  lv_obj_set_size(btn_prev, 80, 40);
  lv_obj_align(btn_prev, LV_ALIGN_BOTTOM_LEFT, 20, -12);
  lv_obj_t *lbl_prev = lv_label_create(btn_prev);
  lv_label_set_text(lbl_prev, "上一张");
  lv_obj_set_style_text_font(lbl_prev, &LXGWWenKaiMono_Light_14, 0);
  lv_obj_center(lbl_prev);
  lv_obj_add_event_cb(btn_prev, btn_prev_cb, LV_EVENT_CLICKED, NULL);

  /* Play/Pause button */
  btn_play_pause = lv_btn_create(ctrl_bar);
  lv_obj_set_size(btn_play_pause, 80, 40);
  lv_obj_align(btn_play_pause, LV_ALIGN_BOTTOM_MID, 0, -12);
  lv_obj_set_style_bg_color(btn_play_pause, lv_color_hex(0x007AFF), 0);
  lbl_play_pause = lv_label_create(btn_play_pause);
  lv_label_set_text(lbl_play_pause, "播放");
  lv_obj_set_style_text_font(lbl_play_pause, &LXGWWenKaiMono_Light_14, 0);
  lv_obj_center(lbl_play_pause);
  lv_obj_add_event_cb(btn_play_pause, btn_play_pause_cb, LV_EVENT_CLICKED,
                      NULL);

  /* Next button */
  lv_obj_t *btn_next = lv_btn_create(ctrl_bar);
  lv_obj_set_size(btn_next, 80, 40);
  lv_obj_align(btn_next, LV_ALIGN_BOTTOM_RIGHT, -20, -12);
  lv_obj_t *lbl_next = lv_label_create(btn_next);
  lv_label_set_text(lbl_next, "下一张");
  lv_obj_set_style_text_font(lbl_next, &LXGWWenKaiMono_Light_14, 0);
  lv_obj_center(lbl_next);
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

  /* Load saved state or start from first image */
  load_gallery_state();
  display_current_image();

  printf("[Gallery] Screen shown with %d images\n", image_count);
} /**
   * @brief Hide gallery and return to previous screen
   */
void ui_gallery_hide(void) {
  /* Stop slideshow if playing */
  if (is_playing) {
    stop_slideshow();
  }

  /* Save current state */
  save_gallery_state();

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
  current_index = 0;
  display_current_image();
}

/**
 * @brief Image click callback - enter fullscreen mode
 */
static void img_click_cb(lv_event_t *e) {
  (void)e;
  enter_fullscreen();
}

/**
 * @brief Display image in fullscreen mode
 */
static void display_fullscreen_image(void) {
  if (!fullscreen_container)
    return;

  /* Clear previous content */
  lv_obj_clean(fullscreen_container);

  /* Create fullscreen image */
  lv_obj_t *img = lv_img_create(fullscreen_container);
  lv_img_set_src(img, image_list[current_index]);
  lv_obj_center(img);

  printf("[Gallery] Fullscreen image %d/%d\n", current_index + 1, image_count);
}

/**
 * @brief Fullscreen mode event handler
 */
static void fullscreen_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_PRESSED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    if (ind)
      lv_indev_get_point(ind, &p);
    touch_start_x = p.x;
    touch_start_y = p.y;
  } else if (code == LV_EVENT_RELEASED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    if (ind)
      lv_indev_get_point(ind, &p);

    int dx = p.x - touch_start_x;
    int dy = p.y - touch_start_y;

    /* Determine swipe direction */
    if (abs(dy) > abs(dx)) {
      /* Vertical swipe */
      if (dy > 80) {
        /* Down swipe: exit fullscreen */
        exit_fullscreen();
      }
    } else {
      /* Horizontal swipe in fullscreen */
      if (dx < -100) {
        /* Left swipe: next image */
        int new_index = current_index + 1;
        if (new_index >= image_count)
          new_index = 0;
        current_index = new_index;
        display_fullscreen_image();
        save_gallery_state();
      } else if (dx > 100) {
        /* Right swipe: previous image */
        int new_index = current_index - 1;
        if (new_index < 0)
          new_index = image_count - 1;
        current_index = new_index;
        display_fullscreen_image();
        save_gallery_state();
      }
    }
  }
}

/**
 * @brief Enter fullscreen mode
 */
static void enter_fullscreen(void) {
  if (is_fullscreen)
    return;

  is_fullscreen = true;

  /* Hide header and control bar */
  if (hdr)
    lv_obj_add_flag(hdr, LV_OBJ_FLAG_HIDDEN);
  if (ctrl_bar)
    lv_obj_add_flag(ctrl_bar, LV_OBJ_FLAG_HIDDEN);
  if (img_container)
    lv_obj_add_flag(img_container, LV_OBJ_FLAG_HIDDEN);

  /* Create fullscreen container */
  fullscreen_container = lv_obj_create(scr_gallery);
  lv_obj_set_size(fullscreen_container, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(fullscreen_container, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(fullscreen_container, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(fullscreen_container, 0, 0);
  lv_obj_set_style_pad_all(fullscreen_container, 0, 0);
  lv_obj_clear_flag(fullscreen_container, LV_OBJ_FLAG_SCROLLABLE);

  /* Add event handler for fullscreen gestures */
  lv_obj_add_event_cb(fullscreen_container, fullscreen_event_cb, LV_EVENT_ALL,
                      NULL);

  /* Display current image in fullscreen */
  display_fullscreen_image();

  printf("[Gallery] Entered fullscreen mode\n");
}

/**
 * @brief Exit fullscreen mode
 */
static void exit_fullscreen(void) {
  if (!is_fullscreen)
    return;

  is_fullscreen = false;

  /* Delete fullscreen container */
  if (fullscreen_container) {
    lv_obj_del(fullscreen_container);
    fullscreen_container = NULL;
  }

  /* Show header and control bar */
  if (hdr)
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_HIDDEN);
  if (ctrl_bar)
    lv_obj_clear_flag(ctrl_bar, LV_OBJ_FLAG_HIDDEN);
  if (img_container)
    lv_obj_clear_flag(img_container, LV_OBJ_FLAG_HIDDEN);

  /* Refresh normal view */
  display_current_image();

  /* Resume slideshow if it was playing */
  if (is_playing) {
    /* Update play/pause button label */
    if (lbl_play_pause) {
      lv_label_set_text(lbl_play_pause, "暂停");
    }
  }

  printf("[Gallery] Exited fullscreen mode\n");
}
