// include/app/ui/ui_music.h
#ifndef UI_MUSIC_H
#define UI_MUSIC_H

#include "lvgl.h"

typedef struct {
  const char *title;
  const char *artist;
  const char *album;
  int duration_seconds;
} song_t;

void ui_music_init(void);
void ui_music_show(void);
void ui_music_hide(void);
void ui_music_set_song(const song_t *s);
void ui_music_set_playing(bool playing);

#endif // UI_MUSIC_H
