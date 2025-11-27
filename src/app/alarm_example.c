// src/app/alarm_example.c
#include "app/alarm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void on_trigger(const alarm_t *a) {
  printf("Alarm triggered: %s (%s)\n", a->id, a->label);
  // attempt to play sound via mplayer (device side). This example just prints.
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  alarm_init(NULL);
  alarm_register_trigger_cb(on_trigger);
  // Simple loop: call check once
  alarm_check_due();
  alarm_shutdown();
  return 0;
}
