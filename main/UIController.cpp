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
#include "UIImageLoader.h"
#include "ui_img_utils.h"
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
}

/**
 * @brief Start LVGL handler task
 * @details Creates FreeRTOS task running lv_timer_handler() at ~50 FPS.
 *          Task priority: tskIDLE_PRIORITY + 5
 */
void UIController::startLVGLTask() {
	// Create LVGL timer handler task (handles GUI updates)
	xTaskCreate(lvglTimerTaskWrapper, "lv_timer_task", 4096, this,
				tskIDLE_PRIORITY + 5, nullptr);
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
		lv_timer_handler();
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
	char versionText[32];
	snprintf(versionText, sizeof(versionText), "V:%s", Application::appVersion);
	lv_label_set_text(ui_VersionStr, versionText);
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
	ui_load_splash_images_wrapper();
	// Refresh logo image source after loading
	if (ui_LogoImg && ui_img_1818877690.data) {
		lv_image_set_src(ui_LogoImg, &ui_img_1818877690);
		ESP_LOGD("UIController", "ui_img_1818877690.header.cf=%d data_size=%u", ui_img_1818877690.header.cf, (unsigned int)ui_img_1818877690.data_size);

		/* Prefer built-in LVGL colorkey as it's more efficient; fall back to alpha conversion if not supported */
		if (ui_img_1818877690.data != NULL) ui_img_apply_colorkey_to_obj(ui_LogoImg, &ui_img_1818877690);
		ESP_LOGD("UIController", "Applied LVGL colorkey (if available) to ui_LogoImg. header.cf=%d data_size=%u", ui_img_1818877690.header.cf, (unsigned int)ui_img_1818877690.data_size);

		lv_obj_invalidate(ui_LogoImg);
	}
	lv_screen_load_anim(ui_Splash, LV_SCR_LOAD_ANIM_FADE_ON, 1000, 0, false);
}

/**
 * @brief Show main sensor screen
 * @details Transitions to main screen with 1s fade animation.
 *          Images are loaded immediately. Frees splash images first to reclaim memory.
 */
void UIController::showMainScreen() {
	ui_free_splash_images_wrapper();
	ui_load_main_images_wrapper();
	/* Apply colorkey to objects once the main images are loaded */
	if (ui_TPMSicon1 && ui_img_tpmsblack_png.data) ui_img_apply_colorkey_to_obj(ui_TPMSicon1, &ui_img_tpmsblack_png);
	if (ui_TPMSicon2 && ui_img_tpmsblack_png.data) ui_img_apply_colorkey_to_obj(ui_TPMSicon2, &ui_img_tpmsblack_png);
	if (ui_BTicon1 && ui_img_btoff_png.data) ui_img_apply_colorkey_to_obj(ui_BTicon1, &ui_img_btoff_png);
	if (ui_BTicon2 && ui_img_btoff_png.data) ui_img_apply_colorkey_to_obj(ui_BTicon2, &ui_img_btoff_png);
	if (ui_Alert1 && ui_img_idle_png.data) ui_img_apply_colorkey_to_obj(ui_Alert1, &ui_img_idle_png);
	if (ui_Alert2 && ui_img_idle_png.data) ui_img_apply_colorkey_to_obj(ui_Alert2, &ui_img_idle_png);
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
void UIController::initializeLabels() {
	State &state = State::getInstance();
	
	// Set unit label based on configuration
	lv_label_set_text(ui_Unit, state.getPressureUnit().c_str());
	
	lv_label_set_text(ui_Pressure1, "---");
	lv_label_set_text(ui_Pressure2, "---");
	lv_label_set_text(ui_TempText1, "-- °C");
	lv_label_set_text(ui_TempText2, "-- °C");
	lv_label_set_text(ui_BatteryText1, "--%");
	lv_label_set_text(ui_BatteryText2, "--%");
	lv_arc_set_value(ui_Battery2, 0);
	lv_arc_set_value(ui_Battery1, 0);
	lv_image_set_src(ui_TPMSicon1, &ui_img_tpmsblack_png);
	if (ui_img_tpmsblack_png.data != NULL) ui_img_apply_colorkey_to_obj(ui_TPMSicon1, &ui_img_tpmsblack_png);
	lv_image_set_src(ui_TPMSicon2, &ui_img_tpmsblack_png);
	if (ui_img_tpmsblack_png.data != NULL) ui_img_apply_colorkey_to_obj(ui_TPMSicon2, &ui_img_tpmsblack_png);
	lv_image_set_src(ui_BTicon1, &ui_img_btoff_png);
	if (ui_img_btoff_png.data != NULL) ui_img_apply_colorkey_to_obj(ui_BTicon1, &ui_img_btoff_png);
	lv_image_set_src(ui_BTicon2, &ui_img_btoff_png);
	if (ui_img_btoff_png.data != NULL) ui_img_apply_colorkey_to_obj(ui_BTicon2, &ui_img_btoff_png);
	lv_image_set_src(ui_Alert2, &ui_img_idle_png);
	if (ui_img_idle_png.data != NULL) ui_img_apply_colorkey_to_obj(ui_Alert2, &ui_img_idle_png);
	lv_image_set_src(ui_Alert1, &ui_img_idle_png);
	if (ui_img_idle_png.data != NULL) ui_img_apply_colorkey_to_obj(ui_Alert1, &ui_img_idle_png);
}

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
void UIController::updateSensorUI(TPMSSensor *frontSensor, TPMSSensor *rearSensor,
							   float frontIdealPSI, float rearIdealPSI,
							   uint32_t currentTime) {	
	bool alertFront = false;
	bool alertRear = false;

	if (frontSensor) {
		updateFrontSensorUI(frontSensor, frontIdealPSI, currentTime);

		alertFront = frontSensor->getAlert();
	} else {
		clearFrontSensorUI(true); // true = apply blink
	}

	if (rearSensor) {
		updateRearSensorUI(rearSensor, rearIdealPSI, currentTime);

		alertRear = rearSensor->getAlert();
	} else {
		clearRearSensorUI(true); // true = apply blink
	}

	updateAlertIcons(alertFront, alertRear);
}

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
void UIController::updateFrontSensorUI(TPMSSensor *frontSensor,
							   float frontIdealPSI,
							   uint32_t currentTime) {
	char buf[16];
	State &state = State::getInstance();
	
	// Display pressure in selected unit
	if (state.getPressureUnit() == "BAR") {
		snprintf(buf, sizeof(buf), "%.2f", frontSensor->getPressureBar());
	} else {
		snprintf(buf, sizeof(buf), "%.1f", frontSensor->getPressurePSI());
	}
	lv_label_set_text(ui_Pressure1, buf);
	
	// Reset label color to white when sensor is synchronized
	lv_obj_set_style_text_color(ui_Pressure1, lv_color_hex(0xFFFFFF),
								LV_PART_MAIN );
	snprintf(buf, sizeof(buf), "%.1f °C", frontSensor->getTemperatureC());
	lv_label_set_text(ui_TempText1, buf);

	snprintf(buf, sizeof(buf), "%d%%", frontSensor->getBatteryLevel());
	lv_label_set_text(ui_BatteryText1, buf);

	lv_arc_set_value(ui_Battery1, static_cast<int>(frontSensor->getBatteryLevel()));
	lv_bar_set_value(ui_BatteryBar1, static_cast<int>(frontSensor->getTemperatureC()),
					 LV_ANIM_ON);

	// Change bar color to blue if temperature is below 10°C
	if (frontSensor->getTemperatureC() < 10.0f) {
		lv_obj_set_style_bg_color(ui_BatteryBar1, lv_color_hex(0x000080),
								  LV_PART_MAIN);
		lv_obj_set_style_bg_color(ui_BatteryBar1, lv_color_hex(0x0000FF),
								  LV_PART_INDICATOR);
	} else {
		lv_obj_set_style_bg_color(ui_BatteryBar1, lv_color_hex(0x183A1B),
								  LV_PART_MAIN);
		lv_obj_set_style_bg_color(ui_BatteryBar1, lv_color_hex(0x00FF13),
								  LV_PART_INDICATOR);
	}

	// Update pressure indicator icon
	if (frontSensor->getPressurePSI() < frontIdealPSI * 0.75f) {
		lv_image_set_src(ui_TPMSicon1, &ui_img_tpmsred_png);
	} else if (frontSensor->getPressurePSI() < frontIdealPSI * 0.9f) {
		lv_image_set_src(ui_TPMSicon1, &ui_img_tpmsyellow_png);
	} else {
		lv_image_set_src(ui_TPMSicon1, &ui_img_tpmsblack_png);
	
	}

	// Update BLE connection status icon
	if (frontSensor->getTimestamp() + 200 < currentTime) {
		lv_image_set_src(ui_BTicon1, &ui_img_btoff_png);
	
	} else {
		lv_image_set_src(ui_BTicon1, &ui_img_bton_png);
	}
}

/**
 * @brief Update rear sensor UI display
 * @param rearSensor Rear tire sensor data
 * @param rearIdealPSI Target pressure for rear tire
 * @param currentTime Current timestamp in milliseconds
 * @details Same logic as updateFrontSensorUI but for rear tire UI elements
 *          (ui_Pressure2, ui_TempText2, ui_BatteryText2, ui_Battery2, ui_BatteryBar2, ui_TPMSicon2, ui_BTicon2)
 */
void UIController::updateRearSensorUI(TPMSSensor *rearSensor, float rearIdealPSI,
							  uint32_t currentTime) {
	char buf[16];
	State &state = State::getInstance();
	
	// Display pressure in selected unit
	if (state.getPressureUnit() == "BAR") {
		snprintf(buf, sizeof(buf), "%.2f", rearSensor->getPressureBar());
	} else {
		snprintf(buf, sizeof(buf), "%.1f", rearSensor->getPressurePSI());
	}
	lv_label_set_text(ui_Pressure2, buf);
	
	// Reset label color to white when sensor is synchronized
	lv_obj_set_style_text_color(ui_Pressure2, lv_color_hex(0xFFFFFF),
								LV_PART_MAIN );
	snprintf(buf, sizeof(buf), "%.1f °C", rearSensor->getTemperatureC());
	lv_label_set_text(ui_TempText2, buf);

	snprintf(buf, sizeof(buf), "%d%%", rearSensor->getBatteryLevel());
	lv_label_set_text(ui_BatteryText2, buf);

	lv_arc_set_value(ui_Battery2, static_cast<int>(rearSensor->getBatteryLevel()));
	lv_bar_set_value(ui_BatteryBar2, static_cast<int>(rearSensor->getTemperatureC()),
					 LV_ANIM_ON);

	// Change bar color to blue if temperature is below 10°C
	if (rearSensor->getTemperatureC() < 10.0f) {
		lv_obj_set_style_bg_color(ui_BatteryBar2, lv_color_hex(0x000080),
								  LV_PART_MAIN);
		lv_obj_set_style_bg_color(ui_BatteryBar2, lv_color_hex(0x0000FF),
								  LV_PART_INDICATOR);
	} else {
		lv_obj_set_style_bg_color(ui_BatteryBar2, lv_color_hex(0x183A1B),
								  LV_PART_MAIN);
		lv_obj_set_style_bg_color(ui_BatteryBar2, lv_color_hex(0x00FF13),
								  LV_PART_INDICATOR);
	}

	// Update pressure indicator icon
	if (rearSensor->getPressurePSI() < rearIdealPSI * 0.75f) {
		lv_image_set_src(ui_TPMSicon2, &ui_img_tpmsred_png);
		if (ui_img_tpmsred_png.data != NULL) ui_img_apply_colorkey_to_obj(ui_TPMSicon2, &ui_img_tpmsred_png);
	} else if (rearSensor->getPressurePSI() < rearIdealPSI * 0.9f) {
		lv_image_set_src(ui_TPMSicon2, &ui_img_tpmsyellow_png);
		if (ui_img_tpmsyellow_png.data != NULL) ui_img_apply_colorkey_to_obj(ui_TPMSicon2, &ui_img_tpmsyellow_png);
	} else {
		lv_image_set_src(ui_TPMSicon2, &ui_img_tpmsblack_png);
		if (ui_img_tpmsblack_png.data != NULL) ui_img_apply_colorkey_to_obj(ui_TPMSicon2, &ui_img_tpmsblack_png);
	}

	// Update BLE connection status icon
	if (rearSensor->getTimestamp() + 200 < currentTime) {
		lv_image_set_src(ui_BTicon2, &ui_img_btoff_png);
		
	} else {
		lv_image_set_src(ui_BTicon2, &ui_img_bton_png);
		
	}
}

/**
 * @brief Clear front sensor UI when sensor is not available
 * @param applyBlink If true, apply 500ms blink effect to pressure label
 * @details Resets all front sensor UI elements to default/empty state.
 *          Blinking (white <-> black) indicates sensor is not synchronized.
 */
void UIController::clearFrontSensorUI(bool applyBlink) {
	lv_label_set_text(ui_Pressure1, "---");

	// Apply blinking effect only if requested: white when blink state is true,
	// black when false
	if (applyBlink) {
		if (m_labelBlinkState) {
			lv_obj_set_style_text_color(ui_Pressure1, lv_color_hex(0xFFFFFF),
										LV_PART_MAIN);
		} else {
			lv_obj_set_style_text_color(ui_Pressure1, lv_color_hex(0x000000),
										LV_PART_MAIN);
		}
	} else {
		// Reset to white when not blinking
		lv_obj_set_style_text_color(ui_Pressure1, lv_color_hex(0xFFFFFF),
									LV_PART_MAIN);
	}

	lv_label_set_text(ui_TempText1, "-- °C");
	lv_label_set_text(ui_BatteryText1, "--%");
	lv_arc_set_value(ui_Battery1, 0);
	lv_bar_set_value(ui_BatteryBar1, -10, LV_ANIM_ON);
	lv_image_set_src(ui_TPMSicon1, &ui_img_tpmsblack_png);
	if (ui_img_tpmsblack_png.data != NULL) ui_img_apply_colorkey_to_obj(ui_TPMSicon1, &ui_img_tpmsblack_png);
	lv_image_set_src(ui_BTicon1, &ui_img_btoff_png);
	if (ui_img_btoff_png.data != NULL) ui_img_apply_colorkey_to_obj(ui_BTicon1, &ui_img_btoff_png);
}

/**
 * @brief Clear rear sensor UI when sensor is not available
 * @param applyBlink If true, apply 500ms blink effect to pressure label
 * @details Same as clearFrontSensorUI but for rear tire UI elements
 */
void UIController::clearRearSensorUI(bool applyBlink) {
	lv_label_set_text(ui_Pressure2, "---");

	// Apply blinking effect only if requested: white when blink state is true,
	// black when false
	if (applyBlink) {
		if (m_labelBlinkState) {
			lv_obj_set_style_text_color(ui_Pressure2, lv_color_hex(0xFFFFFF),
										LV_PART_MAIN);
		} else {
			lv_obj_set_style_text_color(ui_Pressure2, lv_color_hex(0x000000),
										LV_PART_MAIN);
		}
	} else {
		// Reset to white when not blinking
		lv_obj_set_style_text_color(ui_Pressure2, lv_color_hex(0xFFFFFF),
									LV_PART_MAIN);
	}

	lv_label_set_text(ui_TempText2, "-- °C");
	lv_label_set_text(ui_BatteryText2, "--%");
	lv_arc_set_value(ui_Battery2, 0);
	lv_bar_set_value(ui_BatteryBar2, -10, LV_ANIM_ON);
	lv_image_set_src(ui_TPMSicon2, &ui_img_tpmsblack_png);
	if (ui_img_tpmsblack_png.data != NULL) ui_img_apply_colorkey_to_obj(ui_TPMSicon2, &ui_img_tpmsblack_png);
	lv_image_set_src(ui_BTicon2, &ui_img_btoff_png);
	if (ui_img_btoff_png.data != NULL) ui_img_apply_colorkey_to_obj(ui_BTicon2, &ui_img_btoff_png);
}

/**
 * @brief Update alert icons based on sensor alert flags
 * @param alertFront Front sensor alert status
 * @param alertRear Rear sensor alert status
 * @details If any sensor has alert flag set, blinks alert icons (ui_Alert1, ui_Alert2)
 *          at 250ms period. Shows idle icon when no alerts active.
 */
void UIController::updateAlertIcons(bool alertFront, bool alertRear) {
	if (alertFront || alertRear) {
		// Blink: show alert when blink state is true, hide when false
		if (m_alertBlinkState) {
			lv_image_set_src(ui_Alert1, &ui_img_alert_png);
			lv_image_set_src(ui_Alert2, &ui_img_alert_png);
		} else {
			lv_image_set_src(ui_Alert1, &ui_img_idle_png);
			lv_image_set_src(ui_Alert2, &ui_img_idle_png);
		}
	} else {
		// No alert - show idle
		lv_image_set_src(ui_Alert1, &ui_img_idle_png);
		lv_image_set_src(ui_Alert2, &ui_img_idle_png);
	}
}

/**
 * @brief Update blink states for alerts and labels
 * @param currentTime Current timestamp in milliseconds
 * @details Toggles alert blink state every 250ms (for alert icons)
 *          and label blink state every 500ms (for unsynchronized sensor labels)
 */
void UIController::updateAlertBlinkState(uint32_t currentTime) {
	// Toggle blink state every 250ms
	if (currentTime - m_lastBlinkTime >= 250) {
		m_alertBlinkState = !m_alertBlinkState;
		m_lastBlinkTime = currentTime;
	}

	// Toggle label blink state every 500ms
	if (currentTime - m_lastLabelBlinkTime >= 500) {
		m_labelBlinkState = !m_labelBlinkState;
		m_lastLabelBlinkTime = currentTime;
	}
}
