#pragma once

#include "TPMSUtil.h"
#include <cstdint>

class UIController {
public:
	static UIController &instance();

	// Initialization
	void startLVGLTickTimer();
	void startLVGLTask();

	// Screen transitions
	void showSplashScreen();
	void showMainScreen();
	void showPairScreen();

	// UI initialization
	void initializeLabels();

	// UI updates for sensor data
	void updateSensorUI(TPMSUtil *frontSensor, TPMSUtil *rearSensor,
						float frontIdealPSI, float rearIdealPSI,
						uint32_t currentTime);

	// Alert blinking state
	bool getAlertBlinkState() const { return m_alertBlinkState; }
	void updateAlertBlinkState(uint32_t currentTime);

private:
	UIController() = default;
	~UIController() = default;

	// Disable copy/move
	UIController(const UIController &) = delete;
	UIController &operator=(const UIController &) = delete;

	// LVGL task and timer callbacks
	static void lvglTickCallback(void *arg);
	static void lvglTimerTaskWrapper(void *pvParameter);
	void lvglTimerTask();

	// Helper methods for updating specific sensor UI
	void updateFrontSensorUI(TPMSUtil *frontSensor, float frontIdealPSI,
							 uint32_t currentTime);
	void updateRearSensorUI(TPMSUtil *rearSensor, float rearIdealPSI,
							uint32_t currentTime);
	void clearFrontSensorUI();
	void clearRearSensorUI();
	void updateAlertIcons(bool alertFront, bool alertRear);
	void updateSpinner(bool frontSynced, bool rearSynced);

	// Alert blinking state
	bool m_alertBlinkState = false;
	uint32_t m_lastBlinkTime = 0;
};
