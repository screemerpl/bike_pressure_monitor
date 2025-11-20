/**
 * @file SPIFFSManager.h
 * @brief SPIFFS filesystem manager for UI assets
 * @details Handles mounting SPIFFS partition and providing access to embedded PNG images
 * 
 * SPDX-License-Identifier: CC0-1.0
 */

#ifndef SPIFFS_MANAGER_H
#define SPIFFS_MANAGER_H

#include "esp_spiffs.h"
#include "esp_log.h"

/**
 * @brief SPIFFS filesystem manager
 * @details Singleton class managing SPIFFS partition for UI assets (PNG images).
 *          Mounts partition on /spiffs, used by LVGL to load images from filesystem
 *          instead of embedding them as C arrays.
 */
class SPIFFSManager {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to SPIFFSManager instance
     */
    static SPIFFSManager& instance();

    /**
     * @brief Initialize and mount SPIFFS partition
     * @details Mounts "storage" partition to /spiffs mount point.
     *          Must be called before LVGL attempts to load PNG images.
     * @return true if mounted successfully, false on error
     */
    bool init();

    /**
     * @brief Check if SPIFFS is mounted
     * @return true if filesystem is mounted and ready
     */
    bool isMounted() const { return m_mounted; }

    /**
     * @brief Get SPIFFS usage statistics
     * @param[out] totalBytes Total partition size in bytes
     * @param[out] usedBytes Used space in bytes
     * @return true if stats retrieved successfully
     */
    bool getUsage(size_t& totalBytes, size_t& usedBytes);

    /**
     * @brief Unmount SPIFFS partition
     * @details Flushes buffers and unmounts filesystem. Usually called on shutdown.
     */
    void deinit();

    // Prevent copying
    SPIFFSManager(const SPIFFSManager&) = delete;
    SPIFFSManager& operator=(const SPIFFSManager&) = delete;

private:
    SPIFFSManager() : m_mounted(false) {}
    ~SPIFFSManager() { deinit(); }

    static constexpr const char* TAG = "SPIFFS";
    static constexpr const char* MOUNT_POINT = "/spiffs";
    static constexpr const char* PARTITION_LABEL = "storage";

    bool m_mounted; ///< Filesystem mount state
};

#endif // SPIFFS_MANAGER_H
