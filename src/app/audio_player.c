#include "app/audio_player.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static pid_t player_pid = -1;
static int to_player_fd = -1;   // write to player's stdin
static int from_player_fd = -1; // read from player's stdout

static pthread_t reader_thread;
static int cur_pos = 0;
static int cur_len = 0;
static int playing = 0;
static audio_event_cb_t event_cb = NULL;
static char meta_title[256] = "";
static char meta_artist[256] = "";
static char meta_album[256] = "";

static void *reader_fn(void *arg) {
  (void)arg;
  FILE *f = fdopen(from_player_fd, "r");
  if (!f)
    return NULL;
  char line[512];
  while (fgets(line, sizeof(line), f)) {
    // echo mplayer output for debugging
    printf("[mplayer] %s", line);
    // parse simple ANS_* responses or clip info
    if (strncmp(line, "ANS_TIME_POSITION=", 18) == 0) {
      cur_pos = atoi(line + 18);
    } else if (strncmp(line, "ANS_TIME_LENGTH=", 16) == 0) {
      cur_len = atoi(line + 16);
    } else if (strstr(line, "ALBUM:")) {
      const char *p = strchr(line, ':');
      if (p) {
        p++;
        while (*p == ' ' || *p == '\t')
          p++;
        strncpy(meta_album, p, sizeof(meta_album) - 1);
        char *nl = strchr(meta_album, '\n');
        if (nl)
          *nl = '\0';
      }
    } else if (strstr(line, "ARTIST:")) {
      const char *p = strchr(line, ':');
      if (p) {
        p++;
        while (*p == ' ' || *p == '\t')
          p++;
        strncpy(meta_artist, p, sizeof(meta_artist) - 1);
        char *nl = strchr(meta_artist, '\n');
        if (nl)
          *nl = '\0';
      }
    } else if (strstr(line, "TITLE:")) {
      const char *p = strchr(line, ':');
      if (p) {
        p++;
        while (*p == ' ' || *p == '\t')
          p++;
        strncpy(meta_title, p, sizeof(meta_title) - 1);
        char *nl = strchr(meta_title, '\n');
        if (nl)
          *nl = '\0';
      }
    } else if (strstr(line, "EOF code: 0")) {
      // end of file reached
      playing = 0;
      if (event_cb)
        event_cb(1);
    }
    // continue reading
  }
  fclose(f);
  return NULL;
}

bool audio_init(const char *mplayer_path, const char *ao_driver) {
  if (player_pid > 0)
    return true;
  int inpipe[2];
  int outpipe[2];
  if (pipe(inpipe) != 0)
    return false;
  if (pipe(outpipe) != 0) {
    close(inpipe[0]);
    close(inpipe[1]);
    return false;
  }

  // If using OSS, try to reset the device to avoid "OSS: Reset failed" errors
  if (!ao_driver || strcmp(ao_driver, "oss") == 0) {
    int dsp_fd = open("/dev/dsp", O_RDWR | O_NONBLOCK);
    if (dsp_fd >= 0) {
      if (ioctl(dsp_fd, SNDCTL_DSP_RESET, 0) == 0) {
        printf("[Audio] OSS device reset OK\n");
      } else {
        perror("[Audio] OSS ioctl SNDCTL_DSP_RESET failed");
      }
      close(dsp_fd);
    } else {
      perror("[Audio] open /dev/dsp failed");
    }
  }

  pid_t pid = fork();
  if (pid == 0) {
    // child
    dup2(inpipe[0], STDIN_FILENO);
    dup2(outpipe[1], STDOUT_FILENO);
    dup2(outpipe[1], STDERR_FILENO);
    close(inpipe[0]);
    close(inpipe[1]);
    close(outpipe[0]);
    close(outpipe[1]);
    const char *ao = ao_driver ? ao_driver : "oss";
    execlp(mplayer_path ? mplayer_path : "mplayer",
           mplayer_path ? mplayer_path : "mplayer", "-slave", "-idle", "-quiet",
           "-vo", "null", "-ao", ao, (char *)NULL);
    perror("execlp mplayer failed");
    _exit(127);
  }

  // parent
  player_pid = pid;
  close(inpipe[0]);
  close(outpipe[1]);
  to_player_fd = inpipe[1];
  from_player_fd = outpipe[0];

  printf("[Audio] started mplayer pid=%d ao=%s to_fd=%d from_fd=%d\n",
         player_pid, ao_driver ? ao_driver : "oss", to_player_fd,
         from_player_fd);
  // start reader thread
  if (pthread_create(&reader_thread, NULL, reader_fn, NULL) != 0) {
    perror("pthread_create");
  }
  return true;
}

static bool send_cmd(const char *cmd) {
  if (to_player_fd < 0) {
    fprintf(stderr, "[Audio] send_cmd: no to_player_fd\n");
    return false;
  }
  size_t len = strlen(cmd);
  printf("[Audio] send_cmd: %s\n", cmd);
  if (write(to_player_fd, cmd, len) != (ssize_t)len) {
    perror("write to_player_fd");
    return false;
  }
  if (write(to_player_fd, "\n", 1) != 1) {
    perror("write newline");
    return false;
  }
  return true;
}

bool audio_play_file(const char *path) {
  if (!path)
    return false;
  char cmd[1024];
  // quote path to support spaces
  snprintf(cmd, sizeof(cmd), "loadfile \"%s\" 0", path);
  playing = 1;
  return send_cmd(cmd);
}

bool audio_toggle_pause(void) {
  send_cmd("pause");
  return true;
}

bool audio_seek_rel(int seconds) {
  char cmd[64];
  snprintf(cmd, sizeof(cmd), "seek %d 0", seconds);
  return send_cmd(cmd);
}

int audio_get_pos(void) { return cur_pos; }
int audio_get_len(void) { return cur_len; }

bool audio_quit(void) {
  if (player_pid <= 0)
    return true;
  send_cmd("quit");
  int status = 0;
  waitpid(player_pid, &status, 0);
  player_pid = -1;
  if (to_player_fd >= 0) {
    close(to_player_fd);
    to_player_fd = -1;
  }
  if (from_player_fd >= 0) {
    close(from_player_fd);
    from_player_fd = -1;
  }
  return true;
}

void audio_set_event_cb(audio_event_cb_t cb) { event_cb = cb; }

const char *audio_get_title(void) { return meta_title; }
const char *audio_get_artist(void) { return meta_artist; }
const char *audio_get_album(void) { return meta_album; }
