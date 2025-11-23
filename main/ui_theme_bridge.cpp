// Bridge to provide ui_theme_colors and ui_theme_alphas symbols expected by application
// We do not touch auto-generated UI files; instead, export arrays in main/ that
// reference the values defined in generated UI variables.

#include "UI/ui.h"
#include <cstdint>

// Build per-theme color arrays using values from generated variables
static const uint32_t _ui_theme_colors_c0[5] = {
    (uint32_t)_ui_theme_color_Background[0],
    (uint32_t)_ui_theme_color_Text[0],
    (uint32_t)_ui_theme_color_Standard[0],
    (uint32_t)_ui_theme_color_Bright[0],
    (uint32_t)_ui_theme_color_Hot[0]
};
static const uint32_t _ui_theme_colors_c1[5] = {
    (uint32_t)_ui_theme_color_Background[1],
    (uint32_t)_ui_theme_color_Text[1],
    (uint32_t)_ui_theme_color_Standard[1],
    (uint32_t)_ui_theme_color_Bright[1],
    (uint32_t)_ui_theme_color_Hot[1]
};
static const uint32_t _ui_theme_colors_c2[5] = {
    (uint32_t)_ui_theme_color_Background[2],
    (uint32_t)_ui_theme_color_Text[2],
    (uint32_t)_ui_theme_color_Standard[2],
    (uint32_t)_ui_theme_color_Bright[2],
    (uint32_t)_ui_theme_color_Hot[2]
};

static const uint8_t _ui_theme_alphas_c0[5] = {
    (uint8_t)_ui_theme_alpha_Background[0],
    (uint8_t)_ui_theme_alpha_Text[0],
    (uint8_t)_ui_theme_alpha_Standard[0],
    (uint8_t)_ui_theme_alpha_Bright[0],
    (uint8_t)_ui_theme_alpha_Hot[0]
};
static const uint8_t _ui_theme_alphas_c1[5] = {
    (uint8_t)_ui_theme_alpha_Background[1],
    (uint8_t)_ui_theme_alpha_Text[1],
    (uint8_t)_ui_theme_alpha_Standard[1],
    (uint8_t)_ui_theme_alpha_Bright[1],
    (uint8_t)_ui_theme_alpha_Hot[1]
};
static const uint8_t _ui_theme_alphas_c2[5] = {
    (uint8_t)_ui_theme_alpha_Background[2],
    (uint8_t)_ui_theme_alpha_Text[2],
    (uint8_t)_ui_theme_alpha_Standard[2],
    (uint8_t)_ui_theme_alpha_Bright[2],
    (uint8_t)_ui_theme_alpha_Hot[2]
};

// Exported symbols expected by generated UI header - ensure C linkage
#ifdef __cplusplus
extern "C" {
#endif

const uint32_t * ui_theme_colors[3] = {
    _ui_theme_colors_c0,
    _ui_theme_colors_c1,
    _ui_theme_colors_c2
};

const uint8_t * ui_theme_alphas[3] = {
    _ui_theme_alphas_c0,
    _ui_theme_alphas_c1,
    _ui_theme_alphas_c2
};

#ifdef __cplusplus
}
#endif
