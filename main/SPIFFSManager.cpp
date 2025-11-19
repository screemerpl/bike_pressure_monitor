/**
 * @file SPIFFSManager.cpp
 * @brief SPIFFS filesystem manager implementation
 * 
 * SPDX-License-Identifier: CC0-1.0
 */

#include "SPIFFSManager.h"
#include <dirent.h>
#include <stdio.h>

SPIFFSManager& SPIFFSManager::instance() {
    static SPIFFSManager instance;
    return instance;
}

bool SPIFFSManager::init() {
    if (m_mounted) {
        return true;
    }

    esp_vfs_spiffs_conf_t conf = {
        .base_path = MOUNT_POINT,
        .partition_label = PARTITION_LABEL,
        .max_files = 5,              // Max 5 open files simultaneously
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    
    if (ret != ESP_OK) {
        return false;
    }

    m_mounted = true;
    
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
        esp_vfs_spiffs_unregister(PARTITION_LABEL);
        m_mounted = false;
    }
}
