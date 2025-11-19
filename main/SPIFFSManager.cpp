/**
 * @file SPIFFSManager.cpp
 * @brief SPIFFS filesystem manager implementation
 * 
 * SPDX-License-Identifier: CC0-1.0
 */

#include "SPIFFSManager.h"

SPIFFSManager& SPIFFSManager::instance() {
    static SPIFFSManager instance;
    return instance;
}

bool SPIFFSManager::init() {
    if (m_mounted) {
        ESP_LOGW(TAG, "SPIFFS already mounted");
        return true;
    }

    ESP_LOGI(TAG, "Initializing SPIFFS...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = MOUNT_POINT,
        .partition_label = PARTITION_LABEL,
        .max_files = 5,              // Max 5 open files simultaneously
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return false;
    }

    // Check filesystem usage
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(PARTITION_LABEL, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS: %d KB total, %d KB used", total / 1024, used / 1024);
    }

    m_mounted = true;
    ESP_LOGI(TAG, "SPIFFS mounted successfully at %s", MOUNT_POINT);
    return true;
}

bool SPIFFSManager::getUsage(size_t& totalBytes, size_t& usedBytes) {
    if (!m_mounted) {
        return false;
    }

    esp_err_t ret = esp_spiffs_info(PARTITION_LABEL, &totalBytes, &usedBytes);
    return (ret == ESP_OK);
}

void SPIFFSManager::deinit() {
    if (m_mounted) {
        ESP_LOGI(TAG, "Unmounting SPIFFS...");
        esp_vfs_spiffs_unregister(PARTITION_LABEL);
        m_mounted = false;
    }
}
