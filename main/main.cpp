/**
 * @file main.cpp
 * @brief Application entry point
 * @details ESP-IDF entry point for Bike TPMS Monitor application.
 *          Initializes Application singleton and starts all tasks.
 * 
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: CC0-1.0
 */

#include "Application.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Main application entry point
 * @details Called by ESP-IDF after system initialization.
 *          Sequence:
 *          1. Get Application singleton instance
 *          2. Initialize all components (Display, BLE, Config, UI)
 *          3. Start application tasks (LVGL handler, button handler, main loop)
 *          4. Delete app_main task (no longer needed)
 *          
 *          After this function, the application runs in FreeRTOS tasks:
 *          - LVGL timer task (~50 FPS UI updates)
 *          - Button handler task (GPIO interrupts)
 *          - Main application loop (sensor updates, screen management)
 */
extern "C" void app_main(void) {
    // Get application singleton instance
    Application &app = Application::instance();

    // Initialize all components (Display, BLE, NVS, UI)
    app.init();

    // Start application tasks (LVGL, button handler, main loop)
    app.run();

    // Delete app_main task - application now runs in FreeRTOS tasks
    vTaskDelete(nullptr);
}
