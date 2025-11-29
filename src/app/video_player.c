#include "app/video_player.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
static video_event_cb_t event_cb = NULL;
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
    printf("[mplayer-video] %s", line);
    if (strncmp(line, "ANS_TIME_POSITION=", 18) == 0) {
      cur_pos = atoi(line + 18);
    } else if (strncmp(line, "ANS_TIME_LENGTH=", 16) == 0) {
      cur_len = atoi(line + 16);
    } else if (strstr(line, "EOF code: 0")) {
      playing = 0;
      if (event_cb)
        event_cb(1);
    }
  }
  fclose(f);
  return NULL;
}

bool video_init(const char *mplayer_path, const char *vo_driver) {
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
    const char *vo = vo_driver ? vo_driver : "fbdev";
    execlp(mplayer_path ? mplayer_path : "mplayer",
           mplayer_path ? mplayer_path : "mplayer", "-slave", "-idle", "-quiet",
           "-vo", vo, "-ao", "oss", (char *)NULL);
    perror("execlp mplayer failed");
    _exit(127);
  }

  // parent
  player_pid = pid;
  close(inpipe[0]);
  close(outpipe[1]);
  to_player_fd = inpipe[1];
  from_player_fd = outpipe[0];

  printf("[Video] started mplayer pid=%d vo=%s to_fd=%d from_fd=%d\n",
         player_pid, vo_driver ? vo_driver : "fbdev", to_player_fd,
         from_player_fd);
  if (pthread_create(&reader_thread, NULL, reader_fn, NULL) != 0) {
    perror("pthread_create");
  }
  return true;
}

static bool send_cmd(const char *cmd) {
  if (to_player_fd < 0) {
    fprintf(stderr, "[Video] send_cmd: no to_player_fd\n");
    return false;
  }
  size_t len = strlen(cmd);
  printf("[Video] send_cmd: %s\n", cmd);
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

bool video_play_file(const char *path) {
  if (!path)
    return false;
  char cmd[1024];
  snprintf(cmd, sizeof(cmd), "loadfile \"%s\" 0", path);
  playing = 1;
  return send_cmd(cmd);
}

bool video_toggle_pause(void) {
  send_cmd("pause");
  return true;
}

bool video_seek_rel(int seconds) {
  char cmd[64];
  snprintf(cmd, sizeof(cmd), "seek %d 0", seconds);
  return send_cmd(cmd);
}

int video_get_pos(void) { return cur_pos; }
int video_get_len(void) { return cur_len; }

bool video_quit(void) {
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

void video_set_event_cb(video_event_cb_t cb) { event_cb = cb; }

const char *video_get_title(void) { return meta_title; }
const char *video_get_artist(void) { return meta_artist; }
const char *video_get_album(void) { return meta_album; }
