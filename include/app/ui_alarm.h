// include/app/ui_alarm.h
#ifndef UI_ALARM_H
#define UI_ALARM_H

#include "app/alarm.h"

// Initialize alarm UI (create internal screen)
void ui_alarm_init(void);

// Show/hide alarm screen
void ui_alarm_show(void);
void ui_alarm_hide(void);

// Refresh the alarm list view
void ui_alarm_refresh(void);

#endif // UI_ALARM_H
