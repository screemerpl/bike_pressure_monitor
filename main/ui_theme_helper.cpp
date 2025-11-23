#include "ui_theme_helper.h"
#include "UI/ui.h"
#include "lvgl.h"

#include <stdint.h>

uint32_t ui_theme_get_color_uint(uint8_t color_idx) {
    // bounds-check: ui_theme_idx and color_idx are constrained by generated arrays
    if (ui_theme_idx >= 3 || color_idx >= 4) return 0;
    return ui_theme_colors[ui_theme_idx][color_idx];
}

lv_color_t ui_theme_get_lv_color(uint8_t color_idx) {
    return lv_color_hex(ui_theme_get_color_uint(color_idx));
}

uint8_t ui_theme_get_alpha(uint8_t color_idx) {
    if (ui_theme_idx >= 3 || color_idx >= 4) return 255;
    return ui_theme_alphas[ui_theme_idx][color_idx];
}
