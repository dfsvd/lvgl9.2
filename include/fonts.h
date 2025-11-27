/**
 * @file fonts.h
 * @brief 自定义字体声明
 */

#ifndef FONTS_H
#define FONTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "third_party/lvgl/lvgl.h"

/*********************
 * 中文字体 - LXGWWenKaiMono Light
 *********************/
LV_FONT_DECLARE(LXGWWenKaiMono_Light_14);
LV_FONT_DECLARE(LXGWWenKaiMono_Light_18);
LV_FONT_DECLARE(LXGWWenKaiMono_Light_24);

/*********************
 * 数字字体 - Roboto Bold (大号数字)
 *********************/
LV_FONT_DECLARE(Roboto_Bold_150);
LV_FONT_DECLARE(Roboto_Bold_180);
LV_FONT_DECLARE(Roboto_Bold_260);
LV_FONT_DECLARE(Roboto_Bold_320);

/*********************
 * 特殊符号字体 - MapleMono Nerd Font (天气图标等)
 *********************/
LV_FONT_DECLARE(MapleMono_NF_CN_SemiBold_38);
LV_FONT_DECLARE(MapleMono_NF_CN_SemiBold_64);

/*********************
 * PingFang SC 字体（项目新加入）
 *********************/
LV_FONT_DECLARE(PingFangSC_Regular_14);
LV_FONT_DECLARE(PingFangSC_Regular_18);
LV_FONT_DECLARE(PingFangSC_Regular_24);
LV_FONT_DECLARE(PingFangSC_Regular_28);

LV_FONT_DECLARE(PingFangSC_Semibold_38);
LV_FONT_DECLARE(PingFangSC_Semibold_40);
LV_FONT_DECLARE(PingFangSC_Semibold_48);

#ifdef __cplusplus
}
#endif

#endif /* FONTS_H */
