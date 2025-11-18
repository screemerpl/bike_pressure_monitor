#include "Application.h"
#include "State.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include <NimBLEDevice.h>
#include <cstdio>

// Timing constants
static constexpr uint32_t SPLASH_SCREEN_DELAY_MS = 1000;
static constexpr uint32_t MAIN_SCREEN_DELAY_MS = 4500;
static constexpr uint32_t LABEL_INIT_DELAY_MS = 50;
static constexpr uint32_t LONG_PRESS_DURATION_MS = 2000;
static constexpr uint32_t CONTROL_LOOP_DELAY_MS = 100;
static constexpr uint32_t BLE_SCAN_TIME_MS = 1000;

// Default configuration values
static constexpr float DEFAULT_FRONT_PSI = 36.0f;
static constexpr float DEFAULT_REAR_PSI = 42.0f;
static constexpr int DEFAULT_BRIGHTNESS_INDEX = 4;
static constexpr uint8_t MAX_BRIGHTNESS_INDEX = 4;

Application &Application::instance() {
	static Application app;
	return app;
}

void Application::init() {
	esp_log_level_set("*", ESP_LOG_WARN);
	printf("Initializing application...\n");

	loadConfiguration();
	initializeDisplay();
	recordStartTime();
	initBLE();
	startUISystem();

	printf("Application initialized successfully\n");
}

void Application::loadConfiguration() {
	State &state = State::getInstance();

	m_config.init();
	printf("Loaded JSON Config: %s\n", m_config.getJsonString().c_str());

	// Load sensor addresses
	m_config.getString("front_address", state.frontAddress, "80:ea:ca:10:05:32");
	m_config.getString("rear_address", state.rearAddress, "81:ea:ca:20:04:10");

	// Load ideal pressure values
	m_config.getFloat("front_ideal_psi", state.frontIdealPSI, DEFAULT_FRONT_PSI);
	m_config.getFloat("rear_ideal_psi", state.rearIdealPSI, DEFAULT_REAR_PSI);

	// Load and validate brightness setting
	int brightnessIndex = DEFAULT_BRIGHTNESS_INDEX;
	m_config.getInt("brightness_index", brightnessIndex, DEFAULT_BRIGHTNESS_INDEX);
	m_currentBrightnessIndex = static_cast<uint8_t>(brightnessIndex);
	if (m_currentBrightnessIndex > MAX_BRIGHTNESS_INDEX) {
		m_currentBrightnessIndex = MAX_BRIGHTNESS_INDEX;
	}

	// Update pairing status
	state.isPaired = !state.frontAddress.empty() && !state.rearAddress.empty();

	printf("Sensors: Front=%s, Rear=%s, Paired=%d\n",
		   state.frontAddress.c_str(), state.rearAddress.c_str(), state.isPaired);
}

void Application::initializeDisplay() {
	m_display = DisplayManager::instance();
	m_display->init();

	m_uiController = &UIController::instance();

	// Apply saved brightness setting
	m_display->setBacklightBrightness(BRIGHTNESS_LEVELS[m_currentBrightnessIndex]);
	printf("Display brightness: %d%% (index %d)\n",
		   BRIGHTNESS_LEVELS[m_currentBrightnessIndex], m_currentBrightnessIndex);
}

void Application::recordStartTime() {
	m_startTime = esp_timer_get_time() / 1000;
}

void Application::startUISystem() {
	m_uiController->startLVGLTickTimer();
}

void Application::run() {
	// Start LVGL timer handler task (handles GUI updates)
	m_uiController->startLVGLTask();

	// Create control logic task (handles screen transitions and app state)
	xTaskCreate(controlLogicTaskWrapper, "control_logic", 2048, this,
				tskIDLE_PRIORITY + 2, nullptr);
}

void Application::initBLE() {
	printf("Initializing BLE...\n");

	NimBLEDevice::init("");
	NimBLEScan *pBLEScan = NimBLEDevice::getScan();

	pBLEScan->setScanCallbacks(&m_scanCallbacks, false);
	pBLEScan->setActiveScan(true);
	pBLEScan->setMaxResults(0);
	pBLEScan->start(BLE_SCAN_TIME_MS, false, true);

	printf("BLE scanning started\n");
}

void Application::controlLogicTask() {
	bool splashShown = false;
	bool mainShown = false;

	configureButton();

	ButtonState buttonState = {};

	for (;;) {
		uint32_t elapsed = getElapsedTime();

		handleScreenTransitions(elapsed, splashShown, mainShown);

		if (mainShown) {
			handleButtonInput(buttonState);
			updateUIIfPaired();
		}

		vTaskDelay(pdMS_TO_TICKS(CONTROL_LOOP_DELAY_MS));
	}
}

void Application::configureButton() {
	gpio_config_t io_conf = {};
	io_conf.pin_bit_mask = (1ULL << GPIO_NUM_9);
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&io_conf);
}

uint32_t Application::getElapsedTime() const {
	return (esp_timer_get_time() / 1000) - m_startTime;
}

void Application::handleScreenTransitions(uint32_t elapsed, bool &splashShown,
										  bool &mainShown) {
	if (elapsed >= SPLASH_SCREEN_DELAY_MS && !splashShown) {
		lv_async_call(showSplashScreenCallback, nullptr);
		splashShown = true;
		printf("Showing splash screen\n");
	} else if (elapsed >= MAIN_SCREEN_DELAY_MS && !mainShown) {
		lv_async_call(initializeLabelsCallback, nullptr);
		printf("Labels initialized\n");
		vTaskDelay(pdMS_TO_TICKS(LABEL_INIT_DELAY_MS));
		lv_async_call(showMainScreenCallback, nullptr);
		mainShown = true;
		printf("Showing main screen\n");
	}
}

void Application::handleButtonInput(ButtonState &state) {
	bool currentButtonState = gpio_get_level(GPIO_NUM_9);
	uint32_t currentTime = esp_timer_get_time() / 1000;

	if (state.lastState && !currentButtonState) {
		// Button pressed
		state.pressStartTime = currentTime;
		state.pressHandled = false;
		printf("Button pressed\n");
	} else if (!currentButtonState && !state.pressHandled) {
		// Button held down
		uint32_t pressDuration = currentTime - state.pressStartTime;
		if (pressDuration >= LONG_PRESS_DURATION_MS) {
			handleLongPress();
			state.pressHandled = true;
		}
	} else if (!state.lastState && currentButtonState) {
		// Button released
		uint32_t pressDuration = currentTime - state.pressStartTime;
		if (!state.pressHandled && pressDuration < LONG_PRESS_DURATION_MS) {
			handleShortPress();
		}
		state.pressHandled = false;
	}

	state.lastState = currentButtonState;
}

void Application::handleLongPress() {
	printf("Long press detected - switching to Pair screen\n");
	lv_async_call(showPairScreenCallback, nullptr);
}

void Application::handleShortPress() {
	cycleBrightness();
}

void Application::cycleBrightness() {
	m_currentBrightnessIndex = (m_currentBrightnessIndex + 1) % 5;
	uint8_t brightness = BRIGHTNESS_LEVELS[m_currentBrightnessIndex];
	m_display->setBacklightBrightness(brightness);
	m_config.setInt("brightness_index", m_currentBrightnessIndex);
	printf("Brightness set to %d%%\n", brightness);
}

void Application::updateUIIfPaired() {
	State &state = State::getInstance();
	if (state.isPaired) {
		lv_async_call(updateLabelsCallback, nullptr);
	}
}

void Application::showSplashScreenCallback(void *arg) {
	(void)arg;
	UIController::instance().showSplashScreen();
}

void Application::showMainScreenCallback(void *arg) {
	(void)arg;
	UIController::instance().showMainScreen();
}

void Application::showPairScreenCallback(void *arg) {
	(void)arg;
	UIController::instance().showPairScreen();
}

void Application::initializeLabelsCallback(void *arg) {
	(void)arg;
	UIController::instance().initializeLabels();
}

void Application::updateLabelsCallback(void *arg) {
	(void)arg;
	State &state = State::getInstance();
	UIController &ui = UIController::instance();

	// Get sensor data
	TPMSUtil *frontSensor = nullptr;
	TPMSUtil *rearSensor = nullptr;

	auto frontIt = state.data.find(state.frontAddress);
	if (frontIt != state.data.end()) {
		frontSensor = frontIt->second;
	}

	auto rearIt = state.data.find(state.rearAddress);
	if (rearIt != state.data.end()) {
		rearSensor = rearIt->second;
	}

	// Update blink state
	uint32_t currentTime = esp_timer_get_time() / 1000;
	ui.updateAlertBlinkState(currentTime);

	// Update UI with sensor data
	ui.updateSensorUI(frontSensor, rearSensor, state.frontIdealPSI,
					  state.rearIdealPSI, currentTime);
}

void Application::controlLogicTaskWrapper(void *pvParameter) {
	static_cast<Application *>(pvParameter)->controlLogicTask();
}