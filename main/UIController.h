/**
 * @file UIController.h
 * @brief UI update and LVGL task management
 * @details Singleton controller for updating LVGL UI elements with sensor data.
 *          Manages screen transitions, label updates, alert blinking, and
 *          LVGL task/timer lifecycle.
 */

#pragma once

#include "TPMSSensor.h"
#include <cstdint>

/**
 * @class UIController
 * @brief LVGL UI manager and sensor display controller
 * @details Responsibilities:
 *          - Start/manage LVGL tick timer (1ms resolution)
 *          - Run LVGL handler task (~50 FPS)
 *          - Update pressure/temperature/battery UI elements
 *          - Handle alert icon blinking (250ms period)
 *          - Handle label blinking for unsynchronized sensors (500ms period)
 *          - Manage screen transitions (splash, main, pair)
 *          - Apply color coding (green/yellow/red) based on pressure thresholds
 */
class UIController {
public:
	/**
	 * @brief Get singleton instance
	 * @return Reference to UIController singleton
	 */
	static UIController &instance();

	/**
	 * @brief Start LVGL tick timer (1ms periodic)
	 * @details Creates ESP timer calling lv_tick_inc(1) every 1ms for LVGL animations
	 */
	void startLVGLTickTimer();
	
	/**
	 * @brief Start LVGL handler task
	 * @details Creates FreeRTOS task running lv_timer_handler() at ~50 FPS
	 */
	void startLVGLTask();

	/**
	 * @brief Set version label on splash screen
	 * @details Displays "V:X.X.X" from Application::appVersion
	 */
	void setVersionLabel();
	
	/**
	 * @brief Set WiFi mode label
	 * @details Changes label to "WIFI MODE" text
	 */
	void setWiFiModeLabel();
	
	/**
	 * @brief Show splash screen with fade animation
	 */
	void showSplashScreen();
	
	/**
	 * @brief Show main sensor screen with fade animation
	 */
	void showMainScreen();
	
	/**
	 * @brief Show pairing screen with fade animation
	 */
	void showPairScreen();

	/**
	 * @brief Initialize all labels with default values
	 * @details Sets pressure unit label and clears all sensor displays to "---"
	 */
	void initializeLabels();

	/**
	 * @brief Update all sensor UI elements
	 * @param frontSensor Front tire sensor (nullptr if not available)
	 * @param rearSensor Rear tire sensor (nullptr if not available)
	 * @param frontIdealPSI Target pressure for front tire
	 * @param rearIdealPSI Target pressure for rear tire
	 * @param currentTime Current timestamp in milliseconds
	 * @details Updates pressure/temp/battery displays and applies alert blinking.
	 *          Missing sensors trigger label blinking (500ms period).
	 */
	void updateSensorUI(TPMSSensor *frontSensor, TPMSSensor *rearSensor,
					float frontIdealPSI, float rearIdealPSI,
					uint32_t currentTime);	/**
	 * @brief Get current alert blink state
	 * @return true if alert icons should be visible
	 */
	bool getAlertBlinkState() const { return m_alertBlinkState; }
	
	/**
	 * @brief Update alert and label blink states
	 * @param currentTime Current timestamp in milliseconds
	 * @details Toggles alert blink every 250ms, label blink every 500ms
	 */
	void updateAlertBlinkState(uint32_t currentTime);

private:
	UIController() = default;
	~UIController() = default;

	// Disable copy/move
	UIController(const UIController &) = delete;
	UIController &operator=(const UIController &) = delete;

	/**
	 * @brief LVGL tick callback (called every 1ms)
	 * @param arg Unused parameter
	 * @details Increments LVGL internal tick counter for animations
	 */
	static void lvglTickCallback(void *arg);
	
	/**
	 * @brief FreeRTOS task wrapper for LVGL handler
	 * @param pvParameter Pointer to UIController instance
	 */
	static void lvglTimerTaskWrapper(void *pvParameter);
	
	/**
	 * @brief LVGL handler task loop
	 * @details Calls lv_timer_handler() every 20ms (~50 FPS) and
	 *          triggers sensor data cleanup
	 */
	void lvglTimerTask();

	/**
	 * @brief Update front sensor display
	 * @param frontSensor Front tire sensor data
	 * @param frontIdealPSI Target pressure for front tire
	 * @param currentTime Current timestamp in milliseconds
	 * @details Updates pressure (PSI/BAR), temperature, battery level,
	 *          pressure icon (green/yellow/red), BLE status icon,
	 *          and temperature bar color (blue if <10°C)
	 */
	void updateFrontSensorUI(TPMSSensor *frontSensor, float frontIdealPSI,
						 uint32_t currentTime);	/**
	 * @brief Update rear sensor display
	 * @param rearSensor Rear tire sensor data
	 * @param rearIdealPSI Target pressure for rear tire
	 * @param currentTime Current timestamp in milliseconds
	 * @details Same as updateFrontSensorUI but for rear tire UI elements
	 */
	void updateRearSensorUI(TPMSSensor *rearSensor, float rearIdealPSI,
						uint32_t currentTime);	/**
	 * @brief Clear front sensor display
	 * @param applyBlink If true, apply 500ms label blink effect
	 * @details Resets pressure label to "---" and clears temp/battery.
	 *          Blinking indicates sensor is not synchronized.
	 */
	void clearFrontSensorUI(bool applyBlink = false);
	
	/**
	 * @brief Clear rear sensor display
	 * @param applyBlink If true, apply 500ms label blink effect
	 * @details Same as clearFrontSensorUI but for rear tire UI
	 */
	void clearRearSensorUI(bool applyBlink = false);
	
	/**
	 * @brief Update alert icons based on sensor alert flags
	 * @param alertFront Front sensor alert status
	 * @param alertRear Rear sensor alert status
	 * @details Blinks alert icons (250ms period) if any sensor has alert flag
	 */
	void updateAlertIcons(bool alertFront, bool alertRear);

	bool m_alertBlinkState = false;      ///< Alert icon blink state (250ms period)
	uint32_t m_lastBlinkTime = 0;        ///< Last alert blink toggle timestamp
	
	bool m_labelBlinkState = false;      ///< Label blink state (500ms period)
	uint32_t m_lastLabelBlinkTime = 0;   ///< Last label blink toggle timestamp
};
