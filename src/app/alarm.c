// src/app/alarm.c
#include "app/alarm.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Simple dynamic list
static alarm_t *g_alarms = NULL;
static size_t g_count = 0;
static char g_data_dir[512] = "data";
static void (*g_trigger_cb)(const alarm_t *a) = NULL;

static void ensure_data_dir(void) {
  if (g_data_dir[0] == '\0')
    strcpy(g_data_dir, "data");
}

static char *make_data_path(const char *name) {
  ensure_data_dir();
  char *p = malloc(1024);
  snprintf(p, 1024, "%s/%s", g_data_dir, name);
  return p;
}

static void free_alarms_memory(void) {
  free(g_alarms);
  g_alarms = NULL;
  g_count = 0;
}

static void alarm_from_json(const cJSON *item, alarm_t *a) {
  memset(a, 0, sizeof(*a));
  const cJSON *id = cJSON_GetObjectItem(item, "id");
  const cJSON *label = cJSON_GetObjectItem(item, "label");
  const cJSON *enabled = cJSON_GetObjectItem(item, "enabled");
  const cJSON *hour = cJSON_GetObjectItem(item, "hour");
  const cJSON *minute = cJSON_GetObjectItem(item, "minute");
  const cJSON *repeat = cJSON_GetObjectItem(item, "repeat");
  const cJSON *snooze = cJSON_GetObjectItem(item, "snooze_minutes");
  const cJSON *sound = cJSON_GetObjectItem(item, "sound");
  const cJSON *rat = cJSON_GetObjectItem(item, "remove_after_trigger");

  if (id && id->valuestring)
    strncpy(a->id, id->valuestring, ALARM_ID_LEN - 1);
  if (label && label->valuestring)
    strncpy(a->label, label->valuestring, sizeof(a->label) - 1);
  a->enabled = enabled ? enabled->valueint : 0;
  a->hour = hour ? hour->valueint : 0;
  a->minute = minute ? minute->valueint : 0;
  if (repeat && cJSON_IsArray(repeat)) {
    int i;
    for (i = 0; i < 7 && i < cJSON_GetArraySize(repeat); ++i) {
      cJSON *v = cJSON_GetArrayItem(repeat, i);
      a->repeat[i] = v ? v->valueint : 0;
    }
  }
  a->snooze_minutes = snooze ? snooze->valueint : 10;
  if (sound && sound->valuestring)
    strncpy(a->sound, sound->valuestring, sizeof(a->sound) - 1);
  a->remove_after_trigger = rat ? rat->valueint : 0;
}

static cJSON *alarm_to_json(const alarm_t *a) {
  cJSON *item = cJSON_CreateObject();
  cJSON_AddStringToObject(item, "id", a->id);
  cJSON_AddStringToObject(item, "label", a->label);
  cJSON_AddBoolToObject(item, "enabled", a->enabled);
  cJSON_AddNumberToObject(item, "hour", a->hour);
  cJSON_AddNumberToObject(item, "minute", a->minute);
  int vals[7];
  for (int i = 0; i < 7; ++i)
    vals[i] = a->repeat[i] ? 1 : 0;
  cJSON *arr = cJSON_CreateIntArray(vals, 7);
  cJSON_AddItemToObject(item, "repeat", arr);
  cJSON_AddNumberToObject(item, "snooze_minutes", a->snooze_minutes);
  cJSON_AddStringToObject(item, "sound", a->sound);
  cJSON_AddBoolToObject(item, "remove_after_trigger", a->remove_after_trigger);
  return item;
}

int alarm_load(void) {
  char *path = make_data_path("alarms.json");
  FILE *f = fopen(path, "r");
  if (!f) {
    free(path);
    return 0;
  }
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = malloc(sz + 1);
  fread(buf, 1, sz, f);
  buf[sz] = '\0';
  fclose(f);
  cJSON *root = cJSON_Parse(buf);
  free(buf);
  if (!root) {
    free(path);
    return 0;
  }
  if (!cJSON_IsArray(root)) {
    cJSON_Delete(root);
    free(path);
    return 0;
  }
  size_t n = cJSON_GetArraySize(root);
  free_alarms_memory();
  g_alarms = calloc(n, sizeof(alarm_t));
  g_count = n;
  for (size_t i = 0; i < n; ++i) {
    cJSON *it = cJSON_GetArrayItem(root, i);
    alarm_from_json(it, &g_alarms[i]);
  }
  cJSON_Delete(root);
  free(path);
  return 1;
}

int alarm_save_now(void) {
  char *path = make_data_path("alarms.json.tmp");
  char *final = make_data_path("alarms.json");
  cJSON *root = cJSON_CreateArray();
  for (size_t i = 0; i < g_count; ++i) {
    cJSON *it = alarm_to_json(&g_alarms[i]);
    cJSON_AddItemToArray(root, it);
  }
  char *s = cJSON_PrintUnformatted(root);
  FILE *f = fopen(path, "w");
  if (!f) {
    cJSON_Delete(root);
    free(s);
    free(path);
    free(final);
    return 0;
  }
  fwrite(s, 1, strlen(s), f);
  fflush(f);
  fsync(fileno(f));
  fclose(f);
  // rename
  rename(path, final);
  free(s);
  cJSON_Delete(root);
  free(path);
  free(final);
  return 1;
}

int alarm_init(const char *data_dir) {
  if (data_dir && strlen(data_dir) < sizeof(g_data_dir))
    strncpy(g_data_dir, data_dir, sizeof(g_data_dir) - 1);
  ensure_data_dir();
  // attempt load
  alarm_load();
  return 0;
}

void alarm_shutdown(void) {
  alarm_save_now();
  free_alarms_memory();
}

int alarm_add(const alarm_t *a) {
  alarm_t *n = realloc(g_alarms, sizeof(alarm_t) * (g_count + 1));
  if (!n)
    return 0;
  g_alarms = n;
  memcpy(&g_alarms[g_count], a, sizeof(*a));
  g_count++;
  alarm_save_now();
  return 1;
}

int alarm_update(const char *id, const alarm_t *a) {
  for (size_t i = 0; i < g_count; ++i) {
    if (strcmp(g_alarms[i].id, id) == 0) {
      memcpy(&g_alarms[i], a, sizeof(*a));
      alarm_save_now();
      return 1;
    }
  }
  return 0;
}

int alarm_remove(const char *id) {
  size_t dst = 0;
  for (size_t i = 0; i < g_count; ++i) {
    if (strcmp(g_alarms[i].id, id) != 0) {
      g_alarms[dst++] = g_alarms[i];
    }
  }
  if (dst != g_count) {
    g_count = dst;
    g_alarms = realloc(g_alarms, sizeof(alarm_t) * g_count);
    alarm_save_now();
    return 1;
  }
  return 0;
}

int alarm_list(alarm_t **out, size_t *count) {
  if (!out || !count)
    return 0;
  *count = g_count;
  if (g_count == 0) {
    *out = NULL;
    return 1;
  }
  *out = malloc(sizeof(alarm_t) * g_count);
  memcpy(*out, g_alarms, sizeof(alarm_t) * g_count);
  return 1;
}

int alarm_enable(const char *id, bool enable) {
  for (size_t i = 0; i < g_count; ++i) {
    if (strcmp(g_alarms[i].id, id) == 0) {
      g_alarms[i].enabled = enable;
      alarm_save_now();
      return 1;
    }
  }
  return 0;
}

void alarm_register_trigger_cb(void (*cb)(const alarm_t *a)) {
  g_trigger_cb = cb;
}

static int should_trigger(const alarm_t *a, struct tm *tm) {
  if (!a->enabled)
    return 0;
  if (a->hour == tm->tm_hour && a->minute == tm->tm_min) {
    // check repeat
    if (a->repeat[tm->tm_wday])
      return 1;
    // if none repeat set, treat as one-shot
    int any = 0;
    for (int i = 0; i < 7; ++i)
      any |= a->repeat[i];
    if (!any)
      return 1;
  }
  return 0;
}

void alarm_check_due(void) {
  time_t now = time(NULL);
  struct tm tm;
  localtime_r(&now, &tm);
  for (size_t i = 0; i < g_count; ++i) {
    if (should_trigger(&g_alarms[i], &tm)) {
      // If this is a one-shot (no repeat) alarm, disable and persist
      // immediately
      int any = 0;
      for (int k = 0; k < 7; ++k)
        any |= g_alarms[i].repeat[k];
      if (!any && !g_alarms[i].remove_after_trigger) {
        g_alarms[i].enabled = false;
        alarm_save_now();
      }

      if (g_trigger_cb)
        g_trigger_cb(&g_alarms[i]);

      // try to play the configured sound using scripts/play_alarm.sh
      if (g_alarms[i].sound[0]) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "./scripts/play_alarm.sh '%s' &>/dev/null &",
                 g_alarms[i].sound);
        system(cmd);
      }

      if (g_alarms[i].remove_after_trigger) {
        alarm_remove(g_alarms[i].id);
        --i; // shifted
      }
    }
  }
}
