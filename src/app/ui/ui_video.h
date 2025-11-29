// src/app/ui/ui_video.h

#ifndef UI_VIDEO_H
#define UI_VIDEO_H

#include "lvgl.h"

void ui_video_init(void);
void ui_video_show(void);
void ui_video_hide(void);
void ui_video_set_playing(bool playing);
void ui_video_set_time(int elapsed_seconds, int total_seconds);

#endif // UI_VIDEO_H
