#include "UIController.h"
#include "Application.h"
#include "State.h"
#include "UI/ui.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include <cstdio>

UIController &UIController::instance() {
	static UIController controller;
	return controller;
}

void UIController::startLVGLTickTimer() {
	const esp_timer_create_args_t timerArgs = {
		.callback = &lvglTickCallback, .arg = nullptr, .name = "lv_tick"};

	esp_timer_handle_t tickTimer;
	esp_timer_create(&timerArgs, &tickTimer);
	esp_timer_start_periodic(tickTimer, 1000); // 1000 µs = 1 ms
}

void UIController::startLVGLTask() {
	// Create LVGL timer handler task (handles GUI updates)
	xTaskCreate(lvglTimerTaskWrapper, "lv_timer_task", 4096, this,
				tskIDLE_PRIORITY + 5, nullptr);
}

void UIController::lvglTickCallback(void *arg) {
	(void)arg;
	lv_tick_inc(1);
}

void UIController::lvglTimerTask() {
	for (;;) {
		lv_timer_handler();
		vTaskDelay(pdMS_TO_TICKS(20)); // ~50 FPS
		State &state = State::getInstance();
		state.cleanupOldSensors();
	}
}

void UIController::lvglTimerTaskWrapper(void *pvParameter) {
	static_cast<UIController *>(pvParameter)->lvglTimerTask();
}

void UIController::showSplashScreen() {
	char versionText[32];
	snprintf(versionText, sizeof(versionText), "%s", Application::appVersion);
	lv_label_set_text(ui_Label2, versionText);
	lv_screen_load_anim(ui_Splash, LV_SCR_LOAD_ANIM_FADE_ON, 1000, 0, false);
}

void UIController::showMainScreen() {
	lv_screen_load_anim(ui_Main, LV_SCR_LOAD_ANIM_FADE_ON, 1000, 0, false);
}

void UIController::showPairScreen() {
	lv_screen_load_anim(ui_Pair, LV_SCR_LOAD_ANIM_FADE_ON, 1000, 0, false);
}

void UIController::showConfigScreen() {
	// Show splash screen with config mode message
	lv_label_set_text(ui_Label1, "WiFi Config Mode");
	lv_label_set_text(ui_Label2, "Connect to: TPMS-Config");
	lv_screen_load_anim(ui_Splash, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
}

void UIController::initializeLabels() {
	lv_label_set_text(ui_Label3, "---");
	lv_label_set_text(ui_Label4, "---");
	lv_label_set_text(ui_Label5, "-- °C");
	lv_label_set_text(ui_Label6, "-- °C");
	lv_label_set_text(ui_Label7, "--%");
	lv_label_set_text(ui_Label8, "--%");
	lv_arc_set_value(ui_Arc1, 0);
	lv_arc_set_value(ui_Arc2, 0);
	lv_image_set_src(ui_Image1, &ui_img_tpmsblack_png);
	lv_image_set_src(ui_Image3, &ui_img_tpmsblack_png);
	lv_image_set_src(ui_Image6, &ui_img_btoff_png);
	lv_image_set_src(ui_Image7, &ui_img_btoff_png);
	lv_image_set_src(ui_Image9, &ui_img_idle_png);
	lv_image_set_src(ui_Image10, &ui_img_idle_png);
	lv_obj_clear_flag(ui_Spinner1, LV_OBJ_FLAG_HIDDEN);
}

void UIController::updateSensorUI(TPMSUtil *frontSensor, TPMSUtil *rearSensor,
								  float frontIdealPSI, float rearIdealPSI,
								  uint32_t currentTime) {
	bool frontSynced = false;
	bool rearSynced = false;
	bool alertFront = false;
	bool alertRear = false;

	if (frontSensor) {
		updateFrontSensorUI(frontSensor, frontIdealPSI, currentTime);
		frontSynced = true;
		alertFront = frontSensor->alert;
	} else {
		clearFrontSensorUI();
	}

	if (rearSensor) {
		updateRearSensorUI(rearSensor, rearIdealPSI, currentTime);
		rearSynced = true;
		alertRear = rearSensor->alert;
	} else {
		clearRearSensorUI();
	}

	updateSpinner(frontSynced, rearSynced);
	updateAlertIcons(alertFront, alertRear);
}

void UIController::updateFrontSensorUI(TPMSUtil *frontSensor,
									   float frontIdealPSI,
									   uint32_t currentTime) {
	char buf[16];
	snprintf(buf, sizeof(buf), "%.1f", frontSensor->pressurePSI);
	lv_label_set_text(ui_Label3, buf);

	snprintf(buf, sizeof(buf), "%.1f °C", frontSensor->temperatureC);
	lv_label_set_text(ui_Label5, buf);

	snprintf(buf, sizeof(buf), "%d%%", frontSensor->batteryLevel);
	lv_label_set_text(ui_Label7, buf);

	lv_arc_set_value(ui_Arc2, static_cast<int>(frontSensor->batteryLevel));
	lv_bar_set_value(ui_Bar1, static_cast<int>(frontSensor->temperatureC),
					 LV_ANIM_ON);

	// Change bar color to blue if temperature is below 10°C
	if (frontSensor->temperatureC < 10.0f) {
		lv_obj_set_style_bg_color(ui_Bar1, lv_color_hex(0x000080),
								  LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_bg_color(ui_Bar1, lv_color_hex(0x0000FF),
								  LV_PART_INDICATOR | LV_STATE_DEFAULT);
	} else {
		lv_obj_set_style_bg_color(ui_Bar1, lv_color_hex(0x183A1B),
								  LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_bg_color(ui_Bar1, lv_color_hex(0x00FF13),
								  LV_PART_INDICATOR | LV_STATE_DEFAULT);
	}

	// Update pressure indicator icon
	if (frontSensor->pressurePSI < frontIdealPSI * 0.75f) {
		lv_image_set_src(ui_Image1, &ui_img_tpmsred_png);
	} else if (frontSensor->pressurePSI < frontIdealPSI * 0.9f) {
		lv_image_set_src(ui_Image1, &ui_img_tpmsyellow_png);
	} else {
		lv_image_set_src(ui_Image1, &ui_img_tpmsblack_png);
	}

	// Update BLE connection status icon
	if (frontSensor->timestamp + 200 < currentTime) {
		lv_image_set_src(ui_Image6, &ui_img_btoff_png);
	} else {
		lv_image_set_src(ui_Image6, &ui_img_bton_png);
	}
}

void UIController::updateRearSensorUI(TPMSUtil *rearSensor, float rearIdealPSI,
									  uint32_t currentTime) {
	char buf[16];
	snprintf(buf, sizeof(buf), "%.1f", rearSensor->pressurePSI);
	lv_label_set_text(ui_Label4, buf);

	snprintf(buf, sizeof(buf), "%.1f °C", rearSensor->temperatureC);
	lv_label_set_text(ui_Label6, buf);

	snprintf(buf, sizeof(buf), "%d%%", rearSensor->batteryLevel);
	lv_label_set_text(ui_Label8, buf);

	lv_arc_set_value(ui_Arc1, static_cast<int>(rearSensor->batteryLevel));
	lv_bar_set_value(ui_Bar2, static_cast<int>(rearSensor->temperatureC),
					 LV_ANIM_ON);

	// Change bar color to blue if temperature is below 10°C
	if (rearSensor->temperatureC < 10.0f) {
		lv_obj_set_style_bg_color(ui_Bar2, lv_color_hex(0x000080),
								  LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_bg_color(ui_Bar2, lv_color_hex(0x0000FF),
								  LV_PART_INDICATOR | LV_STATE_DEFAULT);
	} else {
		lv_obj_set_style_bg_color(ui_Bar2, lv_color_hex(0x183A1B),
								  LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_bg_color(ui_Bar2, lv_color_hex(0x00FF13),
								  LV_PART_INDICATOR | LV_STATE_DEFAULT);
	}

	// Update pressure indicator icon
	if (rearSensor->pressurePSI < rearIdealPSI * 0.75f) {
		lv_image_set_src(ui_Image3, &ui_img_tpmsred_png);
	} else if (rearSensor->pressurePSI < rearIdealPSI * 0.9f) {
		lv_image_set_src(ui_Image3, &ui_img_tpmsyellow_png);
	} else {
		lv_image_set_src(ui_Image3, &ui_img_tpmsblack_png);
	}

	// Update BLE connection status icon
	if (rearSensor->timestamp + 200 < currentTime) {
		lv_image_set_src(ui_Image7, &ui_img_btoff_png);
	} else {
		lv_image_set_src(ui_Image7, &ui_img_bton_png);
	}
}

void UIController::clearFrontSensorUI() {
	lv_label_set_text(ui_Label3, "---");
	lv_label_set_text(ui_Label5, "-- °C");
	lv_label_set_text(ui_Label7, "--%");
	lv_arc_set_value(ui_Arc2, 0);
	lv_bar_set_value(ui_Bar1, -10, LV_ANIM_ON);
	lv_image_set_src(ui_Image1, &ui_img_tpmsblack_png);
	lv_image_set_src(ui_Image6, &ui_img_btoff_png);
}

void UIController::clearRearSensorUI() {
	lv_label_set_text(ui_Label4, "---");
	lv_label_set_text(ui_Label6, "-- °C");
	lv_label_set_text(ui_Label8, "--%");
	lv_arc_set_value(ui_Arc1, 0);
	lv_bar_set_value(ui_Bar2, -10, LV_ANIM_ON);
	lv_image_set_src(ui_Image3, &ui_img_tpmsblack_png);
	lv_image_set_src(ui_Image7, &ui_img_btoff_png);
}

void UIController::updateAlertIcons(bool alertFront, bool alertRear) {
	if (alertFront || alertRear) {
		// Blink: show alert when blink state is true, hide when false
		if (m_alertBlinkState) {
			lv_image_set_src(ui_Image8, &ui_img_alert_png);
			lv_image_set_src(ui_Image9, &ui_img_alert_png);
		} else {
			lv_image_set_src(ui_Image8, &ui_img_idle_png);
			lv_image_set_src(ui_Image9, &ui_img_idle_png);
		}
	} else {
		// No alert - show idle
		lv_image_set_src(ui_Image8, &ui_img_idle_png);
		lv_image_set_src(ui_Image9, &ui_img_idle_png);
	}
}

void UIController::updateSpinner(bool frontSynced, bool rearSynced) {
	if (rearSynced || frontSynced) {
		lv_obj_add_flag(ui_Spinner1, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_clear_flag(ui_Spinner1, LV_OBJ_FLAG_HIDDEN);
	}
}

void UIController::updateAlertBlinkState(uint32_t currentTime) {
	// Toggle blink state every 250ms
	if (currentTime - m_lastBlinkTime >= 250) {
		m_alertBlinkState = !m_alertBlinkState;
		m_lastBlinkTime = currentTime;
	}
}
