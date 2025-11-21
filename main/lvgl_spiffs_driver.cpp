/**
 * @file lvgl_spiffs_driver.cpp
 * @brief LVGL filesystem driver for SPIFFS
 * @details Registers SPIFFS driver with LVGL to enable image loading from flash
 */

#include "lvgl_spiffs_driver.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <esp_log.h>
#include <cstdio>

static const char* TAG = "LVGL_SPIFFS";

/**
 * @brief Open file callback for LVGL
 */
static void* fs_open_cb(lv_fs_drv_t* drv, const char* path, lv_fs_mode_t mode) {
    (void)drv;
    (void)mode;
    
    // Prepend /spiffs/ to path (LVGL sends path without S: prefix)
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "/spiffs/%s", path);
    
    ESP_LOGI(TAG, "Opening file: %s", full_path);
    
    int flags = O_RDONLY;
    int fd = open(full_path, flags);
    
    if (fd < 0) {
        ESP_LOGE(TAG, "Failed to open file: %s (errno=%d: %s)", full_path, errno, strerror(errno));
        return nullptr;
    }
    
    ESP_LOGI(TAG, "Successfully opened: %s (fd=%d)", full_path, fd);
    // Return file descriptor as void pointer
    return (void*)(intptr_t)fd;
}

/**
 * @brief Close file callback for LVGL
 */
static lv_fs_res_t fs_close_cb(lv_fs_drv_t* drv, void* file_p) {
    (void)drv;
    int fd = (int)(intptr_t)file_p;
    close(fd);
    return LV_FS_RES_OK;
}

/**
 * @brief Read file callback for LVGL
 */
static lv_fs_res_t fs_read_cb(lv_fs_drv_t* drv, void* file_p, void* buf, uint32_t btr, uint32_t* br) {
    (void)drv;
    int fd = (int)(intptr_t)file_p;
    ssize_t result = read(fd, buf, btr);
    
    if (result < 0) {
        *br = 0;
        return LV_FS_RES_UNKNOWN;
    }
    
    *br = (uint32_t)result;
    return LV_FS_RES_OK;
}

/**
 * @brief Seek file callback for LVGL
 */
static lv_fs_res_t fs_seek_cb(lv_fs_drv_t* drv, void* file_p, uint32_t pos, lv_fs_whence_t whence) {
    (void)drv;
    int fd = (int)(intptr_t)file_p;
    int lseek_whence = SEEK_SET;
    
    if (whence == LV_FS_SEEK_CUR) {
        lseek_whence = SEEK_CUR;
    } else if (whence == LV_FS_SEEK_END) {
        lseek_whence = SEEK_END;
    }
    
    off_t result = lseek(fd, pos, lseek_whence);
    if (result < 0) {
        return LV_FS_RES_UNKNOWN;
    }
    
    return LV_FS_RES_OK;
}

/**
 * @brief Tell file position callback for LVGL
 */
static lv_fs_res_t fs_tell_cb(lv_fs_drv_t* drv, void* file_p, uint32_t* pos_p) {
    (void)drv;
    int fd = (int)(intptr_t)file_p;
    off_t result = lseek(fd, 0, SEEK_CUR);
    
    if (result < 0) {
        *pos_p = 0;
        return LV_FS_RES_UNKNOWN;
    }
    
    *pos_p = (uint32_t)result;
    return LV_FS_RES_OK;
}

/**
 * @brief Register LVGL SPIFFS driver
 */
void lvgl_spiffs_driver_register() {
    static lv_fs_drv_t fs_drv;
    lv_fs_drv_init(&fs_drv);
    
    fs_drv.letter = 'S';  // Driver letter (LVGL uses S: for SPIFFS)
    fs_drv.cache_size = 0;
    
    fs_drv.open_cb = fs_open_cb;
    fs_drv.close_cb = fs_close_cb;
    fs_drv.read_cb = fs_read_cb;
    fs_drv.seek_cb = fs_seek_cb;
    fs_drv.tell_cb = fs_tell_cb;
    
    // Write operations not supported
    fs_drv.write_cb = nullptr;
    
    // Directory operations not needed
    fs_drv.dir_open_cb = nullptr;
    fs_drv.dir_read_cb = nullptr;
    fs_drv.dir_close_cb = nullptr;
    
    lv_fs_drv_register(&fs_drv);
    
    ESP_LOGI(TAG, "LVGL SPIFFS driver registered (S:)");
}
