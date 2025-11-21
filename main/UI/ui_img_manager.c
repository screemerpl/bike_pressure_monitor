#include "ui.h"
#include "fastlz.h"
#include "esp_log.h"
#include <inttypes.h>

static const char* TAG = "UI_IMG_MGR";

uint8_t* _ui_load_binary(char* fname, const uint32_t size)
{
    lv_fs_file_t f;
    lv_fs_res_t res;
    res = lv_fs_open(&f, fname, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK) {
        ESP_LOGE(TAG, "Failed to open file: %s (res=%d)", fname, res);
        return NULL;
    }
    uint32_t read_num;
    uint8_t* buf = lv_malloc(sizeof(uint8_t) * size);
    if (buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate %" PRIu32 " bytes for file: %s", size, fname);
        lv_fs_close(&f);
        return NULL;
    }
    res = lv_fs_read(&f, buf, size, &read_num);
    if (res != LV_FS_RES_OK || read_num != size)
    {
        ESP_LOGE(TAG, "Failed to read file: %s (expected %" PRIu32 ", got %" PRIu32 ", res=%d)", 
                 fname, size, read_num, res);
        lv_free(buf);
        lv_fs_close(&f);
        return NULL;
    }
    lv_fs_close(&f);
    return buf;
}


uint8_t* _ui_load_compressed_binary(char* fname, const uint32_t compsize, const uint32_t size )
{
    ESP_LOGI(TAG, "Loading compressed image: %s (compressed=%" PRIu32 ", uncompressed=%" PRIu32 ")", 
             fname, compsize, size);
    
    uint8_t* zip = _ui_load_binary(fname, compsize);
    if (zip == NULL) {
        ESP_LOGE(TAG, "Failed to load compressed file: %s", fname);
        return NULL;
    }
    
    uint8_t* buf = lv_malloc(size);
    if (buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate %" PRIu32 " bytes for decompressed image: %s", size, fname);
        lv_free(zip);
        return NULL;
    }
    
    // Decompress the image data
    size_t outsize = fastlz_decompress(zip, compsize, buf, size);
    
    // Validate decompression result
    if (outsize != size) {
        ESP_LOGE(TAG, "Decompression failed for %s: expected %" PRIu32 " bytes, got %" PRIu32 " bytes", 
                 fname, size, (uint32_t)outsize);
        lv_free(buf);
        lv_free(zip);
        return NULL;
    }
    
    lv_free(zip);
    ESP_LOGI(TAG, "Successfully loaded: %s", fname);
    return buf;
}

// On-demand image loading for splash screen
void ui_load_splash_images(void) {
    ESP_LOGI(TAG, "Loading splash screen images...");
    ui_img_1818877690_load();
}

// On-demand image loading for main screen
// Load images in small batches to avoid memory exhaustion
// Note: This is called from LVGL context, cannot use vTaskDelay
void ui_load_main_images(void) {
    ESP_LOGI(TAG, "Loading main screen images (batch 1)...");
    // First batch - TPMS status icons (critical for display)
    ui_img_tpmsred_png_load();
    if (ui_img_tpmsred_png.data == NULL) {
        ESP_LOGW(TAG, "Failed to load tpmsred, continuing with others");
    }
    
    ui_img_tpmsyellow_png_load();
    if (ui_img_tpmsyellow_png.data == NULL) {
        ESP_LOGW(TAG, "Failed to load tpmsyellow, continuing");
    }
    
    ui_img_tpmsblack_png_load();
    if (ui_img_tpmsblack_png.data == NULL) {
        ESP_LOGW(TAG, "Failed to load tpmsblack, continuing");
    }
    
    // Second batch - Temperature and battery icons
    ESP_LOGI(TAG, "Loading main screen images (batch 2)...");
    ui_img_temp_png_load();
    if (ui_img_temp_png.data == NULL) {
        ESP_LOGW(TAG, "Failed to load temp, continuing");
    }
    
    ui_img_btoff_png_load();
    if (ui_img_btoff_png.data == NULL) {
        ESP_LOGW(TAG, "Failed to load btoff, continuing");
    }
    
    ui_img_bton_png_load();
    if (ui_img_bton_png.data == NULL) {
        ESP_LOGW(TAG, "Failed to load bton, continuing");
    }
    
    // Third batch - Alert icons
    ESP_LOGI(TAG, "Loading main screen images (batch 3)...");
    ui_img_idle_png_load();
    if (ui_img_idle_png.data == NULL) {
        ESP_LOGW(TAG, "Failed to load idle, continuing");
    }
    
    ui_img_alert_png_load();
    if (ui_img_alert_png.data == NULL) {
        ESP_LOGW(TAG, "Failed to load alert, continuing");
    }
    
    ESP_LOGI(TAG, "Main screen images loading complete");
}

// Free splash screen images to reclaim memory
void ui_free_splash_images(void) {
    ESP_LOGI(TAG, "Freeing splash screen images...");
    if (ui_img_1818877690.data != NULL) {
        lv_free((void*)ui_img_1818877690.data);
        ui_img_1818877690.data = NULL;
        ESP_LOGI(TAG, "Freed splash logo image");
    }
}