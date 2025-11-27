// include/app/alarm.h
#ifndef APP_ALARM_H
#define APP_ALARM_H

#include <stdbool.h>
#include <stddef.h>

#define ALARM_ID_LEN 37

typedef struct {
  char id[ALARM_ID_LEN];
  char label[64];
  bool enabled;
  int hour;
  int minute;
  bool repeat[7];
  int snooze_minutes;
  char sound[128];
  bool remove_after_trigger;
} alarm_t;

// Initialize alarm module, data_dir can be NULL to use default "data/"
int alarm_init(const char *data_dir);
void alarm_shutdown(void);

int alarm_add(const alarm_t *a);
int alarm_update(const char *id, const alarm_t *a);
int alarm_remove(const char *id);

// Caller must free *out after use
int alarm_list(alarm_t **out, size_t *count);
int alarm_enable(const char *id, bool enable);

// Register a callback invoked when alarm triggers (can be NULL)
void alarm_register_trigger_cb(void (*cb)(const alarm_t *a));

// Check for due alarms; typically called periodically from main loop
void alarm_check_due(void);

// Helper: force save to disk
int alarm_save_now(void);

#endif // APP_ALARM_H
