#include "ui_img_utils.h"
#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "ui_img_utils";

bool ui_img_get_chroma_key(const lv_image_dsc_t * image_dsc, lv_color_t *out_color) {
    if (!image_dsc) {
        ESP_LOGW(TAG, "ui_img_get_chroma_key: image_dsc is NULL");
        return false;
    }
    if (!out_color) {
        ESP_LOGW(TAG, "ui_img_get_chroma_key: out_color parameter is NULL (image@%p)", (void*)image_dsc);
        return false;
    }
    if (!image_dsc->data) {
        ESP_LOGD(TAG, "ui_img_get_chroma_key: image->data is NULL for image@%p (cf=%d). Probably not loaded yet.", (void*)image_dsc, image_dsc->header.cf);
        return false;
    }
    if (image_dsc->header.cf != LV_COLOR_FORMAT_NATIVE) {
        ESP_LOGD(TAG, "ui_img_get_chroma_key: header.cf != LV_COLOR_FORMAT_NATIVE (cf=%d) for image@%p", image_dsc->header.cf, (void*)image_dsc);
        return false;
    }
    if (image_dsc->header.w <= 0 || image_dsc->header.h <= 0) {
        ESP_LOGW(TAG, "ui_img_get_chroma_key: invalid dimensions w=%d h=%d for image@%p", image_dsc->header.w, image_dsc->header.h, (void*)image_dsc);
        return false;
    }

    /* expecting RGB565 layout (LV_COLOR_DEPTH 16) */
    uint32_t pixels = (uint32_t)image_dsc->header.w * (uint32_t)image_dsc->header.h;
    if (image_dsc->data_size < pixels * 2) {
        ESP_LOGW(TAG, "Chroma: unexpected data_size %u for w=%u h=%u", (unsigned int)image_dsc->data_size, (unsigned int)image_dsc->header.w, (unsigned int)image_dsc->header.h);
        return false;
    }

    uint16_t *pixel_data = (uint16_t *)image_dsc->data;
    uint16_t px = pixel_data[0];

    lv_color16_t chroma16;
    chroma16.red = (px >> 11) & 0x1F;
    chroma16.green = (px >> 5) & 0x3F;
    chroma16.blue = px & 0x1F;

    *out_color = lv_color16_to_color(chroma16);

    ESP_LOGD(TAG, "Detected chroma key RGB565=0x%04X -> RGB(%u,%u,%u) for image@%p (w=%u,h=%u)",
            px, (unsigned int)out_color->red, (unsigned int)out_color->green, (unsigned int)out_color->blue, (void*)image_dsc, (unsigned int)image_dsc->header.w, (unsigned int)image_dsc->header.h);

    return true;
}

bool ui_img_apply_colorkey_to_obj(lv_obj_t *img_obj, const lv_image_dsc_t *image_dsc) {
    if (img_obj == NULL || image_dsc == NULL) return false;

    lv_color_t chroma;
    if (!ui_img_get_chroma_key(image_dsc, &chroma)) {
        /* Not worth alarming the console if an image has no chroma key/different format.
         * Use debug level so expected conditions don't spam logs.
         */
        ESP_LOGD(TAG, "ui_img_get_chroma_key failed for image@%p, skipping colorkey.", (void*)image_dsc);
        return false;
    }

    lv_image_colorkey_t ck = { .low = chroma, .high = chroma };
    lv_obj_set_style_image_colorkey(img_obj, &ck, 0);

    ESP_LOGD(TAG, "Applied LVGL colorkey to object %p with color RGB(%u,%u,%u)", (void*)img_obj, (unsigned int)chroma.red, (unsigned int)chroma.green, (unsigned int)chroma.blue);

    return true;
}
