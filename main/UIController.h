/**
 * @file UIController.h
 * @brief UI update and LVGL task management
 * @details Singleton controller for updating LVGL UI elements with sensor data.
 *          Manages screen transitions, label updates, alert blinking, and
 *          LVGL task/timer lifecycle.
 */

#pragma once

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

	// Sensor display methods moved to UIBikeController. UIController now only handles splash and screen switching.

private:
	UIController() = default;
	~UIController() = default;
	bool m_lvgl_task_started = false;
	bool m_lvgl_timer_started = false;

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


	bool m_versionLabelSet = false;      ///< Set true after version label initialized once
};
