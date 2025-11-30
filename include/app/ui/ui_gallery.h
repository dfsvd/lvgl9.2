// include/app/ui/ui_gallery.h
#ifndef UI_GALLERY_H
#define UI_GALLERY_H

#include "lvgl.h"

/**
 * @brief Initialize the gallery UI screen (called once)
 */
void ui_gallery_init(void);

/**
 * @brief Show the gallery screen (load it as active screen)
 */
void ui_gallery_show(void);

/**
 * @brief Hide the gallery screen and return to previous screen
 */
void ui_gallery_hide(void);

/**
 * @brief Refresh gallery content (reload images if needed)
 */
void ui_gallery_refresh(void);

#endif // UI_GALLERY_H
