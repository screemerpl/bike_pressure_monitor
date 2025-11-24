/**
 * @file UIController.cpp
 * @brief LVGL timing and screen transition controller implementation
 * @details This file implements the UIController singleton responsible for
 *          LVGL timing (tick and task) and screen transitions (splash/main/pair).
 *          The sensor-specific main screen UI updates have been moved to
 *          `UIBikeController` (main/UIBikeController.*) so main-screen logic is
 *          separated from LVGL lifecycle responsibilities.
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

// NOTE: All sensor-specific main screen UI methods (initializeLabels,
// updateSensorUI, updateFrontSensorUI, updateRearSensorUI, clearFrontSensorUI,
// clearRearSensorUI, updateAlertIcons and updateAlertBlinkState) have been
// moved to `UIBikeController` to keep UIController focused on LVGL timing and
// top-level screen transitions. See `main/UIBikeController.*` for details.
