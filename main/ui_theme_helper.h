#pragma once

#include "lvgl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Return the numeric 24-bit RGB color for the active theme and color index. */
uint32_t ui_theme_get_color_uint(uint8_t color_idx);

/** Return the LVGL color object (lv_color_t) for the active theme and color index. */
lv_color_t ui_theme_get_lv_color(uint8_t color_idx);

/** Return the alpha value for the active theme and color index. */
uint8_t ui_theme_get_alpha(uint8_t color_idx);

#ifdef __cplusplus
}
#endif
