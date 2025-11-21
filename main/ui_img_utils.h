#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * Detect chroma key color from pixel (0,0) and return as lv_color_t.
 * Returns true if a chroma key can be derived (image is native RGB without alpha).
 */
bool ui_img_get_chroma_key(const lv_image_dsc_t * image_dsc, lv_color_t *out_color);

/**
 * Apply LVGL built-in colorkey to an object using chroma key detected from image descriptor.
 * If image is not native RGB or the colorkey cannot be derived, returns false.
 */
bool ui_img_apply_colorkey_to_obj(lv_obj_t *img_obj, const lv_image_dsc_t *image_dsc);

#ifdef __cplusplus
} /* extern "C" */
#endif
