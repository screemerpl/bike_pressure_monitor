/**
 * @file UIImageLoader.cpp
 * @brief Wrapper for loading/freeing UI images from SPIFFS
 * @details Provides helper functions for on-demand image loading with detailed logging.
 *          Does not modify auto-generated UI/ files.
 */

#include "UIImageLoader.h"
#include "UI/ui.h"
#include "ui_img_utils.h"
#include "esp_log.h"
#include "esp_heap_caps.h"


static const char* TAG = "UIImageLoader";

void ui_load_splash_images_wrapper(void) {
    ESP_LOGI(TAG, "=== Loading splash screen images ===");
    
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    
    ESP_LOGI(TAG, "Before load: ESP32 heap=%zu bytes, LVGL total=%zu free=%zu used=%zu (%u%%)", 
             free_heap, mon.total_size, mon.free_size, 
             mon.total_size - mon.free_size, mon.used_pct);
    
    // Load splash logo
    ui_img_1818877690_load();
    
    lv_mem_monitor(&mon);
    free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    ESP_LOGI(TAG, "After load: ESP32 heap=%zu bytes, LVGL total=%zu free=%zu used=%zu (%u%%)", 
             free_heap, mon.total_size, mon.free_size,
             mon.total_size - mon.free_size, mon.used_pct);
}

void ui_load_main_images_wrapper(void) {
    ESP_LOGI(TAG, "=== Loading main screen images ===");
    
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    
    ESP_LOGI(TAG, "Before load: ESP32 heap=%zu bytes, LVGL total=%zu free=%zu used=%zu (%u%%)", 
             free_heap, mon.total_size, mon.free_size,
             mon.total_size - mon.free_size, mon.used_pct);
    
    // Load all main screen images
    ui_img_tpmsred_png_load();
    
    ui_img_tpmsyellow_png_load();
    
    ui_img_tpmsblack_png_load();
    
    ui_img_temp_png_load();
    
    ui_img_btoff_png_load();
    
    ui_img_bton_png_load();
    
    ui_img_idle_png_load();
    
    ui_img_alert_png_load();
    
    lv_mem_monitor(&mon);
    free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    ESP_LOGI(TAG, "After load: ESP32 heap=%zu bytes, LVGL total=%zu free=%zu used=%zu (%u%%)", 
             free_heap, mon.total_size, mon.free_size,
             mon.total_size - mon.free_size, mon.used_pct);
}

void ui_free_splash_images_wrapper(void) {
    ESP_LOGI(TAG, "=== Freeing splash screen images ===");
    
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    
    ESP_LOGI(TAG, "Before free: ESP32 heap=%zu bytes, LVGL total=%zu free=%zu used=%zu (%u%%)", 
             free_heap, mon.total_size, mon.free_size,
             mon.total_size - mon.free_size, mon.used_pct);
    
    if (ui_img_1818877690.data != NULL) {
        ESP_LOGI(TAG, "Freeing logo image at %p (35520 bytes)", ui_img_1818877690.data);
        lv_free((void*)ui_img_1818877690.data);
        ui_img_1818877690.data = NULL;
    }
    
    lv_mem_monitor(&mon);
    free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    ESP_LOGI(TAG, "After free: ESP32 heap=%zu bytes, LVGL total=%zu free=%zu used=%zu (%u%%) - reclaimed", 
             free_heap, mon.total_size, mon.free_size,
             mon.total_size - mon.free_size, mon.used_pct);
}
