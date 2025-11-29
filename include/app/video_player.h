// Simple video player interface using mplayer slave mode
#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H

#include <stdbool.h>

bool video_init(const char *mplayer_path, const char *vo_driver);
bool video_play_file(const char *path);
bool video_toggle_pause(void);
bool video_seek_rel(int seconds);
int video_get_pos(void);
int video_get_len(void);
bool video_quit(void);
typedef void (*video_event_cb_t)(int event);
void video_set_event_cb(video_event_cb_t cb);

// Metadata
const char *video_get_title(void);
const char *video_get_artist(void);
const char *video_get_album(void);

#endif // VIDEO_PLAYER_H
