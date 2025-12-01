// src/app/ui/ui_music.c

#include "app/ui/ui_music.h"
#include "app/audio_player.h"
#include "app/network.h"
#include "app/ui_video.h"
#include "cJSON.h"
#include "fonts.h"
#include "lvgl.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define REMOTE_MUSIC_BASE_URL "http://8.217.250.241/music"
#define REMOTE_MUSIC_LIST_URL REMOTE_MUSIC_BASE_URL "/list.json"

static void dump_obj(lv_obj_t *o, const char *name) {
  if (!o) {
    printf("OBJ %s: NULL\n", name);
    return;
  }
  lv_area_t a;
  lv_obj_get_coords(o, &a);
  int w = a.x2 - a.x1 + 1;
  int h = a.y2 - a.y1 + 1;
  printf("OBJ %s: x1=%d y1=%d x2=%d y2=%d w=%d h=%d\n", name, a.x1, a.y1, a.x2,
         a.y2, w, h);
}

static lv_obj_t *scr_music = NULL;
static lv_obj_t *scr_prev = NULL;
static lv_obj_t *scr_remote = NULL;
static lv_coord_t touch_start_x_music = 0;
static lv_coord_t touch_start_y_music = 0;

static lv_obj_t *lbl_title = NULL;
static lv_obj_t *lbl_artist = NULL;
static lv_obj_t *lbl_album = NULL;

// Right column (cover placeholder)
static lv_obj_t *cover_box = NULL;

// Progress
/* elapsed/total time labels removed (no slider) */
static lv_timer_t *play_timer = NULL;
static int elapsed_seconds = 0;
static int total_seconds = 0;
static bool is_playing = false;

// Playlist
static char **playlist = NULL;
static int playlist_count = 0;
static int playlist_index = 0;

static void free_playlist(void) {
  if (!playlist)
    return;
  for (int i = 0; i < playlist_count; ++i)
    free(playlist[i]);
  free(playlist);
  playlist = NULL;
  playlist_count = 0;
  playlist_index = 0;
}

static int scan_music_dir(const char *dir) {
  free_playlist();
  playlist_count = 0;
  playlist = NULL;
  if (!dir)
    return 0;
  DIR *d = opendir(dir);
  if (!d)
    return 0;
  struct dirent *ent;
  while ((ent = readdir(d)) != NULL) {
    if (ent->d_type == DT_REG) {
      // naive filter for common audio extensions
      const char *name = ent->d_name;
      const char *ext = strrchr(name, '.');
      if (!ext)
        continue;
      if (strcasecmp(ext, ".flac") == 0 || strcasecmp(ext, ".mp3") == 0 ||
          strcasecmp(ext, ".wav") == 0) {
        playlist = realloc(playlist, sizeof(char *) * (playlist_count + 1));
        char *full = malloc(strlen(dir) + 1 + strlen(name) + 1);
        sprintf(full, "%s/%s", dir, name);
        playlist[playlist_count++] = full;
      }
    }
  }
  closedir(d);
  return playlist_count;
}

static int file_exists(const char *path) {
  if (!path)
    return 0;
  return access(path, F_OK) == 0;
}

// ensure current playlist_index points to an existing file; if not, try to
// advance or rescan the directory
static int ensure_valid_index(void) {
  if (playlist_count == 0)
    return 0;
  // try current index first
  for (int i = 0; i < playlist_count; ++i) {
    int idx = (playlist_index + i) % playlist_count;
    if (file_exists(playlist[idx])) {
      playlist_index = idx;
      return 1;
    }
  }
  // nothing exists -> try rescanning once
  if (scan_music_dir("/root/data/music") > 0) {
    // try again
    for (int i = 0; i < playlist_count; ++i) {
      if (file_exists(playlist[i])) {
        playlist_index = i;
        return 1;
      }
    }
  }
  return 0;
}

// Controls
static lv_obj_t *btn_prev = NULL;
static lv_obj_t *btn_play = NULL;
static lv_obj_t *btn_next = NULL;
static lv_obj_t *lbl_play_sym = NULL;
static lv_obj_t *btn_remote = NULL;
static lv_obj_t *btn_refresh = NULL;

// Remote download screen
static lv_obj_t *remote_list_container = NULL;
static lv_coord_t touch_start_y_remote = 0;
static bool is_downloading = false;
static char download_path[512];
static long download_expected_size = 0;
static lv_obj_t *download_btn_active = NULL;
static lv_timer_t *download_poll_timer = NULL;

static void format_time(int s, char *buf, size_t len) {
  int m = s / 60;
  int sec = s % 60;
  snprintf(buf, len, "%d:%02d", m, sec);
}

static void play_timer_cb(lv_timer_t *t) {
  (void)t;
  if (!is_playing)
    return;
  if (elapsed_seconds < total_seconds) {
    elapsed_seconds++;
  } else {
    // stop when finished
    is_playing = false;
    if (lbl_play_sym)
      lv_label_set_text(lbl_play_sym, LV_SYMBOL_PLAY);
  }
}

// metadata poll timer to update labels from mplayer clip info
static lv_timer_t *meta_timer = NULL;
static void meta_timer_cb(lv_timer_t *t) {
  (void)t;
  if (!is_playing)
    return;
  const char *titol = audio_get_title();
  const char *art = audio_get_artist();
  const char *alb = audio_get_album();
  char buf[256];
  if (titol && titol[0]) {
    snprintf(buf, sizeof(buf), "歌曲名 : %s", titol);
    lv_label_set_text(lbl_title, buf);
  }
  if (art && art[0]) {
    snprintf(buf, sizeof(buf), "艺术家 : %s", art);
    lv_label_set_text(lbl_artist, buf);
  }
  if (alb && alb[0]) {
    snprintf(buf, sizeof(buf), "专辑 : %s", alb);
    lv_label_set_text(lbl_album, buf);
  }
}

static void play_event_cb(lv_event_t *e) {
  (void)e;
  // toggle: if not started, play file; else toggle pause
  if (playlist_count == 0)
    return;
  if (!is_playing) {
    if (!ensure_valid_index())
      return;
    const char *path = playlist[playlist_index];
    audio_play_file(path);
    is_playing = true;
    if (lbl_play_sym)
      lv_label_set_text(lbl_play_sym, LV_SYMBOL_PAUSE);
    // update UI metadata if available (naive)
    // set placeholders until metadata arrives
    lv_label_set_text(lbl_title, "歌曲名 : 占位");
    lv_label_set_text(lbl_artist, "艺术家 : 占位");
    lv_label_set_text(lbl_album, "专辑 : 占位");
  } else {
    audio_toggle_pause();
    is_playing = !is_playing;
    if (lbl_play_sym)
      lv_label_set_text(lbl_play_sym,
                        is_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
  }
}

static void prev_event_cb(lv_event_t *e) {
  (void)e;
  if (playlist_count == 0)
    return;
  playlist_index = (playlist_index - 1 + playlist_count) % playlist_count;
  if (!ensure_valid_index())
    return;
  audio_play_file(playlist[playlist_index]);
  is_playing = true;
  if (lbl_play_sym)
    lv_label_set_text(lbl_play_sym, LV_SYMBOL_PAUSE);
}

static void next_event_cb(lv_event_t *e) {
  (void)e;
  if (playlist_count == 0)
    return;
  playlist_index = (playlist_index + 1) % playlist_count;
  if (!ensure_valid_index())
    return;
  audio_play_file(playlist[playlist_index]);
  is_playing = true;
  if (lbl_play_sym)
    lv_label_set_text(lbl_play_sym, LV_SYMBOL_PAUSE);
}

static void audio_event_handler(int event) {
  // event 1 = EOF -> play next
  if (event == 1) {
    if (playlist_count == 0)
      return;
    playlist_index = (playlist_index + 1) % playlist_count;
    if (!ensure_valid_index())
      return;
    audio_play_file(playlist[playlist_index]);
  }
}

// Forward declaration
static void ui_remote_show(void);
static void rebuild_playlist_after_download(void);

static void music_overlay_event(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_GESTURE) {
    lv_indev_t *ind = lv_indev_get_act();
    if (!ind)
      return;
    lv_dir_t dir = lv_indev_get_gesture_dir(ind);
    if (dir == LV_DIR_RIGHT) {
      ui_music_hide();
    } else if (dir == LV_DIR_LEFT) {
      ui_video_show();
    } else if (dir == LV_DIR_TOP) {
      ui_remote_show();
    }
    return;
  }

  if (code == LV_EVENT_PRESSED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    if (ind)
      lv_indev_get_point(ind, &p);
    touch_start_x_music = p.x;
    touch_start_y_music = p.y;
  } else if (code == LV_EVENT_RELEASED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    if (ind)
      lv_indev_get_point(ind, &p);
    if (p.x - touch_start_x_music > 180) { /* right swipe threshold */
      ui_music_hide();
      return;
    }
    if (touch_start_x_music - p.x > 180) { /* left swipe threshold */
      ui_video_show();
      return;
    }
    if (touch_start_y_music - p.y > 180) { /* up swipe threshold */
      ui_remote_show();
      return;
    }
  }
}

void ui_music_init(void) {
  if (scr_music)
    return;
  audio_init(NULL, NULL);
  // register event callback for end-of-track
  audio_set_event_cb(audio_event_handler);
  // scan music directory for tracks
  scan_music_dir("/root/data/music");
  scr_music = lv_obj_create(NULL);
  /* Fixed layout: overall screen 800x480 */
  lv_obj_set_size(scr_music, 800, 480);
  lv_obj_remove_style_all(scr_music);

  /* Top box: 800x380, black, no border */
  lv_obj_t *top_box = lv_obj_create(scr_music);
  lv_obj_set_size(top_box, 800, 380);
  lv_obj_set_pos(top_box, 0, 0);
  lv_obj_set_style_bg_color(top_box, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(top_box, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(top_box, 0, 0);
  lv_obj_set_style_radius(top_box, 0, 0);

  /* left column in top box for text */
  lv_obj_t *left_col = lv_obj_create(top_box);
  lv_obj_remove_style_all(left_col);
  lv_obj_set_size(left_col, 460, 300);
  lv_obj_set_pos(left_col, 8, 18);

  lbl_title = lv_label_create(left_col);
  lv_label_set_text(lbl_title, "歌曲名 : 占位");
  lv_obj_set_style_text_color(lbl_title, lv_color_white(), 0);
  lv_obj_set_style_text_font(lbl_title, &LXGWWenKaiMono_Light_24, 0);
  lv_obj_set_pos(lbl_title, 4, 8);

  lbl_artist = lv_label_create(left_col);
  lv_label_set_text(lbl_artist, "艺术家 : 占位");
  lv_obj_set_style_text_color(lbl_artist, lv_color_hex(0x9ca3af), 0);
  lv_obj_set_style_text_font(lbl_artist, &LXGWWenKaiMono_Light_18, 0);
  lv_obj_set_pos(lbl_artist, 4, 44);

  lbl_album = lv_label_create(left_col);
  lv_label_set_text(lbl_album, "专辑 : 占位");
  lv_obj_set_style_text_color(lbl_album, lv_color_hex(0x9ca3af), 0);
  lv_obj_set_style_text_font(lbl_album, &LXGWWenKaiMono_Light_18, 0);
  lv_obj_set_pos(lbl_album, 4, 78);

  /* cover on right side of top box */
  cover_box = lv_obj_create(top_box);
  lv_obj_set_size(cover_box, 160, 160);
  lv_obj_set_style_bg_color(cover_box, lv_color_hex(0x374151), 0);
  lv_obj_set_style_radius(cover_box, 0, 0);
  // lv_obj_set_pos(cover_box, 632, 110);
  lv_obj_align(cover_box, LV_ALIGN_RIGHT_MID, -12, 0);

  /* Bottom box: 800x100, white, no border */
  lv_obj_t *bottom_box = lv_obj_create(scr_music);
  lv_obj_set_size(bottom_box, 800, 100);
  lv_obj_set_pos(bottom_box, 0, 380);
  lv_obj_set_style_bg_color(bottom_box, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(bottom_box, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(bottom_box, 0, 0);
  lv_obj_set_style_radius(bottom_box, 0, 0);

  /* bottom contents: controls only (no time labels or slider) */

  /* control buttons */
  btn_prev = lv_btn_create(bottom_box);
  lv_obj_set_size(btn_prev, 42, 42);
  lv_obj_set_pos(btn_prev, 260, 5);
  lv_obj_add_event_cb(btn_prev, prev_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lprev = lv_label_create(btn_prev);
  lv_label_set_text(lprev, LV_SYMBOL_PREV);

  btn_play = lv_btn_create(bottom_box);
  lv_obj_set_size(btn_play, 42, 42);
  lv_obj_set_pos(btn_play, 360, 5);
  lv_obj_add_event_cb(btn_play, play_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lplay = lv_label_create(btn_play);
  lv_label_set_text(lplay, LV_SYMBOL_PLAY);
  lbl_play_sym = lplay;

  btn_next = lv_btn_create(bottom_box);
  lv_obj_set_size(btn_next, 42, 42);
  lv_obj_set_pos(btn_next, 460, 5);
  lv_obj_add_event_cb(btn_next, next_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lnext = lv_label_create(btn_next);
  lv_label_set_text(lnext, LV_SYMBOL_NEXT);

  /* create timer for simulate progress if not already */
  if (!play_timer) {
    play_timer = lv_timer_create(play_timer_cb, 1000, NULL);
    lv_timer_pause(play_timer);
  }

  if (!meta_timer) {
    meta_timer = lv_timer_create(meta_timer_cb, 200, NULL);
  }

  // If we have a playlist, show first track name
  if (playlist_count > 0) {
    lv_label_set_text(lbl_title, "歌曲名 : 占位");
    lv_label_set_text(lbl_artist, "艺术家 : 占位");
    lv_label_set_text(lbl_album, "专辑 : 占位");
  }

  /* Debug dump: print coordinates to help find overflow */
  dump_obj(scr_music, "scr_music");
  dump_obj(top_box, "top_box");
  dump_obj(bottom_box, "bottom_box");
  dump_obj(left_col, "left_col");
  dump_obj(cover_box, "cover_box");
  dump_obj(btn_prev, "btn_prev");
  dump_obj(btn_play, "btn_play");
  dump_obj(btn_next, "btn_next");

  /* bind swipe detection so children still receive clicks */
  lv_obj_add_event_cb(scr_music, music_overlay_event, LV_EVENT_ALL, NULL);
}

void ui_music_set_song(const song_t *s) {
  if (!s)
    return;
  lv_label_set_text(lbl_title, s->title ? s->title : "");
  lv_label_set_text(lbl_artist, s->artist ? s->artist : "");
  lv_label_set_text(lbl_album, s->album ? s->album : "");
  total_seconds = s->duration_seconds;
  elapsed_seconds = 0;
  char buf[16];
  format_time(total_seconds, buf, sizeof(buf));
  (void)buf;
}

void ui_music_set_playing(bool playing) {
  is_playing = playing;
  if (is_playing) {
    if (lbl_play_sym)
      lv_label_set_text(lbl_play_sym, LV_SYMBOL_PAUSE);
    lv_timer_resume(play_timer);
  } else {
    if (lbl_play_sym)
      lv_label_set_text(lbl_play_sym, LV_SYMBOL_PLAY);
    lv_timer_pause(play_timer);
  }
}

void ui_music_show(void) {
  if (!scr_music)
    ui_music_init();
  scr_prev = lv_scr_act();
  /* animate screen in from right -> left movement */
  lv_scr_load_anim(scr_music, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
  /* Force an immediate refresh so coordinates are computed, then dump */
  lv_refr_now(NULL);
  dump_obj(scr_music, "scr_music");
  /* try to find children by name if they exist */
  lv_obj_t *top_box = lv_obj_get_child(scr_music, 0);
  lv_obj_t *bottom_box = lv_obj_get_child(scr_music, 1);
  dump_obj(top_box, "top_box");
  dump_obj(bottom_box, "bottom_box");
  /* also dump known widgets */
  dump_obj(lbl_title, "lbl_title");
  dump_obj(lbl_artist, "lbl_artist");
  dump_obj(lbl_album, "lbl_album");
  dump_obj(cover_box, "cover_box");
  dump_obj(btn_prev, "btn_prev");
  dump_obj(btn_play, "btn_play");
  dump_obj(btn_next, "btn_next");
}

void ui_music_hide(void) {
  if (scr_prev) {
    /* animate previous screen in from left -> right movement */
    lv_scr_load_anim(scr_prev, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
    scr_prev = NULL;
  }
}

// Remote download helpers
static void download_poll_cb(lv_timer_t *t) {
  (void)t;
  if (!is_downloading || !download_btn_active)
    return;

  FILE *f = fopen(download_path, "rb");
  if (!f) {
    // 文件尚未创建，继续等待
    return;
  }

  fseek(f, 0, SEEK_END);
  long current_size = ftell(f);
  fclose(f);

  if (current_size >= download_expected_size && download_expected_size > 0) {
    // 下载完成
    lv_label_set_text(lv_obj_get_child(download_btn_active, 0), "完成");
    is_downloading = false;
    download_btn_active = NULL;
    if (download_poll_timer) {
      lv_timer_del(download_poll_timer);
      download_poll_timer = NULL;
    }
    rebuild_playlist_after_download();
  } else if (download_expected_size > 0) {
    // 显示进度百分比
    int percent = (int)((current_size * 100) / download_expected_size);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d%%", percent);
    lv_label_set_text(lv_obj_get_child(download_btn_active, 0), buf);
  }
}

static void rebuild_playlist_after_download(void) {
  int prev_count = playlist_count;
  scan_music_dir("/root/data/music");
  if (playlist_count > prev_count) {
    playlist_index = playlist_count - 1;
  }
}

static void download_track_cb(lv_event_t *e) {
  if (is_downloading) {
    // 已有下载任务，禁止并发
    return;
  }

  const char *name = (const char *)lv_event_get_user_data(e);
  if (!name)
    return;

  // 从用户数据中获取预期大小（需要修改 build_remote_list_ui 传递）
  // 暂时从 JSON 重新解析或使用全局缓存，这里简化为直接启动

  char url[512];
  snprintf(url, sizeof(url), "%s/%s", REMOTE_MUSIC_BASE_URL, name);
  snprintf(download_path, sizeof(download_path), "/root/data/music/%s", name);

  lv_obj_t *btn = lv_event_get_target(e);
  lv_label_set_text(lv_obj_get_child(btn, 0), "0%");

  int rc = network_download_file_bg(url, download_path);
  if (rc == 0) {
    is_downloading = true;
    download_btn_active = btn;
    // 启动轮询定时器（每秒检查一次）
    if (!download_poll_timer) {
      download_poll_timer = lv_timer_create(download_poll_cb, 1000, NULL);
    }
  } else {
    lv_label_set_text(lv_obj_get_child(btn, 0), "失败");
  }
}

static void build_remote_list_ui(const char *json) {
  if (!scr_remote)
    return;
  if (remote_list_container) {
    lv_obj_del(remote_list_container);
    remote_list_container = NULL;
  }
  remote_list_container = lv_obj_create(scr_remote);
  lv_obj_set_size(remote_list_container, 780, 430);
  lv_obj_set_pos(remote_list_container, 10, 40);
  lv_obj_set_style_bg_color(remote_list_container, lv_color_white(), 0);
  lv_obj_set_style_border_width(remote_list_container, 0, 0);
  lv_obj_set_style_pad_all(remote_list_container, 4, 0);
  lv_obj_set_scrollbar_mode(remote_list_container, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_scroll_dir(remote_list_container, LV_DIR_VER);

  if (!json) {
    lv_obj_t *lbl = lv_label_create(remote_list_container);
    lv_label_set_text(lbl, "获取失败");
    return;
  }
  cJSON *root = cJSON_Parse(json);
  if (!root || !cJSON_IsArray(root)) {
    lv_obj_t *lbl = lv_label_create(remote_list_container);
    lv_label_set_text(lbl, "解析失败");
    cJSON_Delete(root);
    return;
  }
  int y = 0;
  cJSON *it = NULL;
  cJSON_ArrayForEach(it, root) {
    cJSON *n = cJSON_GetObjectItem(it, "name");
    if (!cJSON_IsString(n))
      continue;
    const char *fname = n->valuestring;
    char line[256];
    cJSON *sz = cJSON_GetObjectItem(it, "size");
    if (cJSON_IsNumber(sz)) {
      int mb = (int)(sz->valuedouble / (1024 * 1024));
      snprintf(line, sizeof(line), "%s (%dMB)", fname, mb);
    } else {
      snprintf(line, sizeof(line), "%s", fname);
    }
    lv_obj_t *row = lv_obj_create(remote_list_container);
    lv_obj_set_size(row, 770, 100);
    lv_obj_set_pos(row, 2, y);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, line);
    lv_obj_set_pos(lbl, 6, 10);
    lv_obj_set_style_text_font(lbl, &PingFangSC_Regular_18, 0);
    lv_obj_t *btn_dl = lv_btn_create(row);
    lv_obj_set_size(btn_dl, 70, 30);
    lv_obj_set_pos(btn_dl, 650, 0);
    char *name_copy = strdup(fname);
    lv_obj_add_event_cb(btn_dl, download_track_cb, LV_EVENT_CLICKED,
                        (void *)strdup(fname));
    lv_obj_t *lbl_dl = lv_label_create(btn_dl);
    lv_label_set_text(lbl_dl, "下载");
    y += 50;
  }
  cJSON_Delete(root);
}

static void remote_overlay_event(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_GESTURE) {
    if (is_downloading) {
      // 下载中，禁止退出
      return;
    }
    lv_indev_t *ind = lv_indev_get_act();
    if (!ind)
      return;
    lv_dir_t dir = lv_indev_get_gesture_dir(ind);
    if (dir == LV_DIR_BOTTOM) {
      scan_music_dir("/root/data/music");
      ui_music_show();
    }
    return;
  }
  if (code == LV_EVENT_PRESSED) {
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    if (ind)
      lv_indev_get_point(ind, &p);
    touch_start_y_remote = p.y;
  } else if (code == LV_EVENT_RELEASED) {
    if (is_downloading) {
      // 下载中，禁止退出
      return;
    }
    lv_indev_t *ind = lv_indev_get_act();
    lv_point_t p;
    if (ind)
      lv_indev_get_point(ind, &p);
    if (p.y - touch_start_y_remote > 180) {
      scan_music_dir("/root/data/music");
      ui_music_show();
    }
  }
}

static void ui_remote_show(void) {
  if (!scr_remote) {
    scr_remote = lv_obj_create(NULL);
    lv_obj_set_size(scr_remote, 800, 480);
    lv_obj_set_style_bg_color(scr_remote, lv_color_hex(0xe5e7eb), 0);
    lv_obj_set_style_bg_opa(scr_remote, LV_OPA_COVER, 0);
    lv_obj_t *title = lv_label_create(scr_remote);
    lv_label_set_text(title, "远程音乐下载");
    lv_obj_set_style_text_font(title, &PingFangSC_Semibold_38, 0);
    lv_obj_set_pos(title, 20, 0);
    lv_obj_add_event_cb(scr_remote, remote_overlay_event, LV_EVENT_ALL, NULL);
  }
  lv_scr_load_anim(scr_remote, LV_SCR_LOAD_ANIM_MOVE_TOP, 300, 0, false);
  lv_obj_t *loading = lv_label_create(scr_remote);
  lv_label_set_text(loading, "加载中...");
  lv_obj_set_pos(loading, 20, 60);
  char *resp = network_fetch_data(REMOTE_MUSIC_LIST_URL);
  if (resp) {
    lv_obj_del(loading);
  }
  build_remote_list_ui(resp);
  if (resp)
    free(resp);
}
