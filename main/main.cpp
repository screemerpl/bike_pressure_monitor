/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "Application.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main(void) {
    // Get application instance
    Application &app = Application::instance();

    // Initialize all components
    app.init();

    // Start application tasks
    app.run();

    // app_main task no longer needed - tasks are running
    vTaskDelete(nullptr);
}
