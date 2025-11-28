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
// Event callback: event code 1 = end-of-file (track finished)
typedef void (*audio_event_cb_t)(int event);
void audio_set_event_cb(audio_event_cb_t cb);
// Metadata getters (may be empty until mplayer reports them)
const char *audio_get_title(void);
const char *audio_get_artist(void);
const char *audio_get_album(void);

#endif // AUDIO_PLAYER_H
