#pragma once

#include "ConfigManager.h"
#include "DisplayManager.h"
#include "PairController.h"
#include "TPMSScanCallbacks.h"
#include "UIController.h"
#include "WiFiManager.h"
#include "WebServer.h"
#include <cstdint>

class Application {
public:
#ifdef GIT_VERSION
	static constexpr char appVersion[] = GIT_VERSION;
#else
	static constexpr char appVersion[] = "1.0.0-dev";
#endif

	static Application &instance();

	void init();
	void run();

	ConfigManager &getConfig() { return m_config; }
	uint32_t getStartTime() const { return m_startTime; }

private:
	Application() = default;
	~Application() = default;

	Application(const Application &) = delete;
	Application &operator=(const Application &) = delete;

	// Initialization helpers
	void loadConfiguration();
	void initializeDisplay();
	void recordStartTime();
	void startUISystem();
	void initBLE();
	void startConfigServer();

	// Main control task
	void controlLogicTask();

	// Control task helpers
	void configureButton();
	uint32_t getElapsedTime() const;
	void handleScreenTransitions(uint32_t elapsed, bool &splashShown,
								 bool &mainShown);

	// Button state tracking
	struct ButtonState {
		bool lastState = true;
		uint32_t pressStartTime = 0;
		bool pressHandled = false;
	};

	void handleButtonInput(ButtonState &state);
	void handleLongPress();
	void handleShortPress();
	void cycleBrightness();
	void updateUIIfPaired();

	// LVGL async callbacks
	static void setVersionLabelCallback(void *arg);
	static void showSplashScreenCallback(void *arg);
	static void showMainScreenCallback(void *arg);
	static void showPairScreenCallback(void *arg);
	static void initializeLabelsCallback(void *arg);
	static void updateLabelsCallback(void *arg);

	// FreeRTOS task wrapper
	static void controlLogicTaskWrapper(void *pvParameter);

	// Member variables
	ConfigManager m_config;
	DisplayManager *m_display = nullptr;
	UIController *m_uiController = nullptr;
	PairController *m_pairController = nullptr;
	TPMSScanCallbacks m_scanCallbacks;
	uint32_t m_startTime = 0;

	static constexpr uint8_t BRIGHTNESS_LEVELS[5] = {10, 30, 50, 75, 100};
	uint8_t m_currentBrightnessIndex = 4;
};