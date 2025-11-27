/**
 * @file hello.c
 * @brief cJSON + 中文字体测试示例
 */

#include "fonts.h"
#include "third_party/cjson/cJSON.h"
#include "third_party/lvgl/lvgl.h"
#include <string.h>

/**
 * @brief 创建测试界面
 */
void hello_app_create(void) {
  // 创建一个容器
  lv_obj_t *container = lv_obj_create(lv_scr_act());
  lv_obj_set_size(container, LV_HOR_RES, LV_VER_RES);
  lv_obj_center(container);
  lv_obj_set_style_bg_color(container, lv_color_hex(0x1E1E2E), 0);

  // 测试 JSON 数据
  const char *json_str =
      "{"
      "\"name\": \"LVGL测试\","
      "\"version\": \"9.2.3\","
      "\"features\": [\"中文支持\", \"JSON解析\", \"自定义字体\"],"
      "\"temperature\": 25.5"
      "}";

  cJSON *json = cJSON_Parse(json_str);
  if (json == NULL) {
    LV_LOG_ERROR("JSON parse failed");
    return;
  }

  // 解析 JSON 数据
  cJSON *name = cJSON_GetObjectItem(json, "name");
  cJSON *version = cJSON_GetObjectItem(json, "version");
  cJSON *temp = cJSON_GetObjectItem(json, "temperature");
  cJSON *features = cJSON_GetObjectItem(json, "features");

  // 创建标题 - 使用大号中文字体
  lv_obj_t *title = lv_label_create(container);
  if (name && cJSON_IsString(name)) {
    lv_label_set_text(title, name->valuestring);
  }
  lv_obj_set_style_text_font(title, &LXGWWenKaiMono_Light_24, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0xCDD6F4), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

  // 创建版本信息 - 使用中号中文字体
  lv_obj_t *ver_label = lv_label_create(container);
  if (version && cJSON_IsString(version)) {
    lv_label_set_text_fmt(ver_label, "版本: %s", version->valuestring);
  }
  lv_obj_set_style_text_font(ver_label, &LXGWWenKaiMono_Light_18, 0);
  lv_obj_set_style_text_color(ver_label, lv_color_hex(0xA6ADC8), 0);
  lv_obj_align_to(ver_label, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

  // 创建温度显示 - 使用大号数字字体
  lv_obj_t *temp_label = lv_label_create(container);
  if (temp && cJSON_IsNumber(temp)) {
    lv_label_set_text_fmt(temp_label, "%.1f°C", temp->valuedouble);
  }
  lv_obj_set_style_text_font(temp_label, &Roboto_Bold_150, 0);
  lv_obj_set_style_text_color(temp_label, lv_color_hex(0xF38BA8), 0);
  lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, -20);

  // 创建功能列表 - 使用小号中文字体
  if (features && cJSON_IsArray(features)) {
    lv_obj_t *features_label = lv_label_create(container);
    char features_text[256] = "功能特性:\n";

    cJSON *feature = NULL;
    cJSON_ArrayForEach(feature, features) {
      if (cJSON_IsString(feature)) {
        strcat(features_text, "• ");
        strcat(features_text, feature->valuestring);
        strcat(features_text, "\n");
      }
    }

    lv_label_set_text(features_label, features_text);
    lv_obj_set_style_text_font(features_label, &LXGWWenKaiMono_Light_14, 0);
    lv_obj_set_style_text_color(features_label, lv_color_hex(0x94E2D5), 0);
    lv_obj_align(features_label, LV_ALIGN_BOTTOM_MID, 0, -30);
  }

  // 清理 JSON 对象
  cJSON_Delete(json);
}
