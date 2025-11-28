// Simple audio player interface using mplayer slave mode
#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <stdbool.h>

bool audio_init(const char *mplayer_path, const char *ao_driver);
bool audio_play_file(const char *path);
bool audio_toggle_pause(void);
bool audio_seek_rel(int seconds);
int audio_get_pos(void);
int audio_get_len(void);
bool audio_quit(void);

#endif // AUDIO_PLAYER_H
