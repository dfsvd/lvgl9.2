#include "app_config.h"
#include "hello.h"
#include "third_party/lvgl/lvgl.h"
#include <unistd.h>

static void lv_linux_disp_init(void) {
  lv_display_t *disp = lv_linux_fbdev_create();
  lv_linux_fbdev_set_file(disp, FRAMEBUFFER_DEVICE);
}

void lv_touchpad_init(void) {
  // 创建触摸屏设备，并关联到屏幕
  lv_indev_t *touchpad =
      lv_evdev_create(LV_INDEV_TYPE_POINTER, TOUCHSCREEN_DEVICE);
  lv_indev_set_disp(touchpad, lv_disp_get_default());
}

int main(void) {
  lv_init();

  /*Linux display device init*/
  lv_linux_disp_init();
  lv_touchpad_init();

  /*Create Hello test app*/
  hello_app_create();

  /*Handle LVGL tasks*/
  while (1) {
    lv_timer_handler();
    usleep(5000);
  }

  return 0;
}
