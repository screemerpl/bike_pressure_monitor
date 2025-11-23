/**
 * @file lvgl_spiffs_driver.h
 * @brief LVGL filesystem driver for SPIFFS
 */

#pragma once

#include "lvgl.h"

/**
 * @brief Register LVGL SPIFFS driver
 * @details Enables LVGL to load files from SPIFFS using "S:" prefix
 */
void lvgl_spiffs_driver_register();
