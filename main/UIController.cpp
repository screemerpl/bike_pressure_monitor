/**
 * @file UIController.cpp
 * @brief UI controller implementation
 * @details Implements LVGL UI updates for TPMS sensor display.
 *          Handles pressure thresholds, color coding, blinking effects,
 *          and unit conversions (PSI/BAR).
 */

#include "UIController.h"
#include "Application.h"
#include "State.h"
#include "UI/ui.h"
#include "UI/ui_themes.h"
#include "ui_theme_helper.h"
#include "UIBikeController.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "esp_log.h"
#include <cstdio>

/**
 * @brief Get singleton instance
 * @return Reference to UIController singleton (static local variable)
 */
UIController &UIController::instance() {
	static UIController controller;
	return controller;
}

/**
 * @brief Start LVGL tick timer
 * @details Creates ESP32 timer calling lv_tick_inc(1) every 1ms.
 *          Required for LVGL animations and timeouts.
 */
void UIController::startLVGLTickTimer() {
	if (m_lvgl_timer_started) return;
	const esp_timer_create_args_t timerArgs = {
		.callback = &lvglTickCallback, 
		.arg = nullptr, 
		.dispatch_method = ESP_TIMER_TASK,
		.name = "lv_tick",
		.skip_unhandled_events = false
	};

	esp_timer_handle_t tickTimer;
	esp_timer_create(&timerArgs, &tickTimer);
	esp_timer_start_periodic(tickTimer, 1000); // 1000 µs = 1 ms
	m_lvgl_timer_started = true;
}

/**
 * @brief Start LVGL handler task
 * @details Creates FreeRTOS task running lv_timer_handler() at ~50 FPS.
 *          Task priority: tskIDLE_PRIORITY + 5
 */
void UIController::startLVGLTask() {
	if (m_lvgl_task_started) return;
	// Create LVGL timer handler task (handles GUI updates)
	xTaskCreate(lvglTimerTaskWrapper, "lv_timer_task", 4096, this,
				tskIDLE_PRIORITY + 5, nullptr);
	m_lvgl_task_started = true;
}

/**
 * @brief LVGL tick callback
 * @param arg Unused parameter
 * @details Called every 1ms by ESP timer to increment LVGL tick counter
 */
void UIController::lvglTickCallback(void *arg) {
	(void)arg;
	lv_tick_inc(1);
}

/**
 * @brief LVGL handler task loop
 * @details Runs lv_timer_handler() every 20ms (~50 FPS) and triggers
 *          sensor data cleanup to remove old/stale sensor entries
 */
void UIController::lvglTimerTask() {
	for (;;) {
		static int iter_count = 0;
		lv_timer_handler();
		if ((iter_count++ % 250) == 0) { // log approximately every 5 seconds
			ESP_LOGD("UIController", "lvglTimerTask alive");
		}
		vTaskDelay(pdMS_TO_TICKS(20)); // ~50 FPS
		State &state = State::getInstance();
		state.cleanupOldSensors();
	}
}

/**
 * @brief FreeRTOS task wrapper
 * @param pvParameter Pointer to UIController instance
 * @details Static wrapper calling non-static lvglTimerTask() method
 */
void UIController::lvglTimerTaskWrapper(void *pvParameter) {
	static_cast<UIController *>(pvParameter)->lvglTimerTask();
}

/**
 * @brief Set version label on splash screen
 * @details Formats "V:X.X.X" text from Application::appVersion constant
 */
void UIController::setVersionLabel() {
	if (ui_VersionStr == NULL) {
		ESP_LOGW("UIController", "setVersionLabel called before ui_VersionStr created");
		return;
	}
	if (m_versionLabelSet) {
		ESP_LOGD("UIController", "Version label already set; skipping");
		return;
	}
	char versionText[32];
	snprintf(versionText, sizeof(versionText), "V:%s", Application::appVersion);
	ESP_LOGD("UIController", "Setting version label to %s", versionText);
	lv_label_set_text(ui_VersionStr, versionText);
	m_versionLabelSet = true;
}

/**
 * @brief Set WiFi mode label
 * @details Changes label text to "WIFI MODE" (used during config portal)
 */
void UIController::setWiFiModeLabel() {
	lv_label_set_text(ui_VersionStr, "WIFI MODE");
}

/**
 * @brief Show splash screen
 * @details Transitions to splash screen with 1s fade animation
 */
void UIController::showSplashScreen() {

	lv_screen_load_anim(ui_Splash, LV_SCR_LOAD_ANIM_FADE_ON, 1000, 0, false);
}

/**
 * @brief Show main sensor screen
 * @details Transitions to main screen with 1s fade animation.
 *          Images are loaded immediately. Frees splash images first to reclaim memory.
 */
void UIController::showMainScreen() {
	lv_screen_load_anim(ui_Main, LV_SCR_LOAD_ANIM_FADE_ON, 1000, 0, false);
}

/**
 * @brief Show pairing screen
 * @details Transitions to pairing screen with 1s fade animation
 */
void UIController::showPairScreen() {
	lv_screen_load_anim(ui_Pair, LV_SCR_LOAD_ANIM_FADE_ON, 1000, 0, false);
}

/**
 * @brief Initialize all UI labels with default values
 * @details Sets pressure unit label from config and clears all
 *          sensor displays to "---", resets arcs/bars to 0, and
 *          sets icons to default (black TPMS, BT off, idle alert)
 */
// initializeLabels moved to UIBikeController

/**
 * @brief Update all sensor UI elements
 * @param frontSensor Front tire sensor (nullptr if not synchronized)
 * @param rearSensor Rear tire sensor (nullptr if not synchronized)
 * @param frontIdealPSI Target pressure for front tire
 * @param rearIdealPSI Target pressure for rear tire
 * @param currentTime Current timestamp in milliseconds
 * @details Updates both front and rear sensor displays, applies blinking
 *          to unsynchronized sensors, and updates alert icons
 */
// updateSensorUI moved to UIBikeController

/**
 * @brief Update front sensor UI display
 * @param frontSensor Front tire sensor data
 * @param frontIdealPSI Target pressure for front tire
 * @param currentTime Current timestamp in milliseconds
 * @details Updates pressure (PSI or BAR based on State::pressureUnit),
 *          temperature, battery level, pressure status icon:
 *          - Red: pressure < 75% of ideal
 *          - Yellow: pressure < 90% of ideal
 *          - Black: pressure >= 90% of ideal
 *          Temperature bar color: Blue if temp < 10°C, green otherwise
 *          BLE icon: ON if data received within last 200ms
 */
// updateFrontSensorUI moved to UIBikeController

/**
 * @brief Update rear sensor UI display
 * @param rearSensor Rear tire sensor data
 * @param rearIdealPSI Target pressure for rear tire
 * @param currentTime Current timestamp in milliseconds
 * @details Same logic as updateFrontSensorUI but for rear tire UI elements
 *          (ui_Pressure2, ui_TempText2, ui_BatteryText2, ui_Battery2, ui_BatteryBar2, ui_TPMSicon2, ui_BTicon2)
 */
// updateRearSensorUI moved to UIBikeController

/**
 * @brief Clear front sensor UI when sensor is not available
 * @param applyBlink If true, apply 500ms blink effect to pressure label
 * @details Resets all front sensor UI elements to default/empty state.
 *          Blinking (white <-> black) indicates sensor is not synchronized.
 */
// clearFrontSensorUI moved to UIBikeController

/**
 * @brief Clear rear sensor UI when sensor is not available
 * @param applyBlink If true, apply 500ms blink effect to pressure label
 * @details Same as clearFrontSensorUI but for rear tire UI elements
 */
// clearRearSensorUI moved to UIBikeController

/**
 * @brief Update alert icons based on sensor alert flags
 * @param alertFront Front sensor alert status
 * @param alertRear Rear sensor alert status
 * @details If any sensor has alert flag set, blinks alert icons (ui_Alert1, ui_Alert2)
 *          at 250ms period. Shows idle icon when no alerts active.
 */
// updateAlertIcons moved to UIBikeController

/**
 * @brief Update blink states for alerts and labels
 * @param currentTime Current timestamp in milliseconds
 * @details Toggles alert blink state every 250ms (for alert icons)
 *          and label blink state every 500ms (for unsynchronized sensor labels)
 */
// updateAlertBlinkState moved to UIBikeController
