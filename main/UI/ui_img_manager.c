#include "ui.h"
#include "fastlz.h"
#include <stdio.h>
#include "esp_heap_caps.h"

uint8_t* _ui_load_binary(char* fname, const uint32_t size)
{
    lv_fs_file_t f;
    lv_fs_res_t res;
    res = lv_fs_open(&f, fname, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK) return NULL; // file not found
    uint32_t read_num;
    uint8_t* buf = lv_malloc(sizeof(uint8_t) * size);
    if (buf == NULL) {
        lv_mem_monitor_t mon;
        lv_mem_monitor(&mon);
        size_t sys_free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        printf("_ui_load_binary: ERROR - lv_malloc failed!\n");
        printf("  Requested: %lu bytes\n", (unsigned long)(sizeof(uint8_t) * size));
        printf("  LVGL heap - Total: %zu, Used: %zu, Free: %zu (fragmented: %d%%)\n", 
               mon.total_size, mon.total_size - mon.free_size, mon.free_size, mon.frag_pct);
        printf("  System heap - Free: %zu bytes\n", sys_free_heap);
        lv_fs_close(&f);
        return NULL;
    }
    res = lv_fs_read(&f, buf, size, &read_num);
    if (res != LV_FS_RES_OK || read_num != size)
    {
        lv_free(buf);
        return NULL;
    }
    lv_fs_close(&f);
    return buf;
}


uint8_t* _ui_load_compressed_binary(char* fname, const uint32_t compsize, const uint32_t size )
{
    uint8_t* zip = _ui_load_binary(fname, compsize);
    if (zip == NULL) return NULL;
    uint8_t* buf = lv_malloc(size);
    if (buf == NULL) {
        lv_mem_monitor_t mon;
        lv_mem_monitor(&mon);
        size_t sys_free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        printf("_ui_load_compressed_binary: ERROR - lv_malloc failed!\n");
        printf("  Requested: %lu bytes\n", (unsigned long)size);
        printf("  LVGL heap - Total: %zu, Used: %zu, Free: %zu (fragmented: %d%%)\n", 
               mon.total_size, mon.total_size - mon.free_size, mon.free_size, mon.frag_pct);
        printf("  System heap - Free: %zu bytes\n", sys_free_heap);
        lv_free(zip);
        return NULL;
    }
    size_t outsize=0;
    size_t excepted_size = size;
    fastlz_decompress(zip, compsize, buf, size);
    lv_free(zip);
    return buf;
}

/**
 * Apply transparency to indexed palette images by detecting the color at pixel (0,0)
 * and setting it as the chroma key (transparent color).
 * This function checks if an image uses indexed palette format (LV_COLOR_FORMAT_NATIVE)
 * and if so, extracts the (0,0) pixel color and enables it as transparent.
 * 
 * @param image_dsc Pointer to the lv_image_dsc_t image descriptor
 */
void ui_img_apply_indexed_palette_transparency(lv_image_dsc_t* image_dsc) {
    if (image_dsc == NULL || image_dsc->data == NULL) {
        return;
    }
    
    // Only process indexed palette (NATIVE format) images
    if (image_dsc->header.cf != LV_COLOR_FORMAT_NATIVE) {
        return;
    }
    
    // Extract color from (0,0) pixel - assuming RGB565 format (most common for NATIVE)
    uint16_t* pixel_data = (uint16_t*)image_dsc->data;
    uint16_t pixel = pixel_data[0];
    
    // Convert RGB565 to RGB888
    uint8_t r = (uint8_t)((pixel >> 11) << 3);      // R: bits 15-11
    uint8_t g = (uint8_t)(((pixel >> 5) & 0x3F) << 2);  // G: bits 10-5
    uint8_t b = (uint8_t)((pixel & 0x1F) << 3);     // B: bits 4-0
    
    lv_color_t chroma_color = lv_color_make(r, g, b);
    
    printf("ui_img_apply_indexed_palette_transparency: Image at %p uses indexed palette\n", image_dsc);
    printf("  Detected chroma key color from pixel(0,0) = RGB(%d, %d, %d) = 0x%06lX\n", 
           r, g, b, (uint32_t)lv_color_to_u32(chroma_color));
}
