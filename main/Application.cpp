/**
 * @file Application.cpp
 * @brief Main application controller implementation
 * @details Orchestrates all subsystems: display, BLE scanning, sensor pairing,
 *          button input, WiFi configuration mode, and screen transitions
 */

#include "Application.h"
#include "State.h"             // Global state singleton
#include "SPIFFSManager.h"     // SPIFFS filesystem for PNG images
#include "driver/gpio.h"       // GPIO configuration for button
#include "esp_timer.h"         // High-resolution timer for timestamps
#include "esp_log.h"           // ESP logging
#include "freertos/FreeRTOS.h" // FreeRTOS primitives
#include "freertos/task.h"     // Task creation and delays
#include "lvgl.h"              // LVGL async calls
#include <NimBLEDevice.h>      // BLE scanning

// Timing constants (milliseconds)
static constexpr uint32_t SPLASH_SCREEN_DELAY_MS = 1000;     ///< Delay before showing splash screen
static constexpr uint32_t MAIN_SCREEN_DELAY_MS = 4500;       ///< Delay before showing main/pair screen
static constexpr uint32_t LABEL_INIT_DELAY_MS = 50;          ///< Delay after initializing labels
static constexpr uint32_t LONG_PRESS_DURATION_MS = 2000;     ///< Duration for long press (clear pairing)
static constexpr uint32_t VERY_LONG_PRESS_DURATION_MS = 15000; ///< Duration for very long press (WiFi mode)
static constexpr uint32_t CONTROL_LOOP_DELAY_MS = 100;       ///< Main control loop iteration delay
static constexpr uint32_t BLE_SCAN_TIME_MS = 1000;           ///< BLE scan window duration

// Default configuration values
static constexpr float DEFAULT_FRONT_PSI = 36.0f;            ///< Default front tire pressure
static constexpr float DEFAULT_REAR_PSI = 42.0f;             ///< Default rear tire pressure
static constexpr int DEFAULT_BRIGHTNESS_INDEX = 4;           ///< Default brightness (100%)
static constexpr uint8_t MAX_BRIGHTNESS_INDEX = 4;           ///< Maximum brightness index

/// Log tag for Application module
[[maybe_unused]] static const char* TAG = "Application";

/**
 * @brief Get singleton instance (Meyer's singleton)
 * @return Reference to the Application singleton
 */
Application &Application::instance() {
	static Application app;
	return app;
}

/**
 * @brief Initialize application subsystems
 * @details Initialization flow:
 *          1. Load configuration from NVS
 *          2. Check for WiFi config mode flag
 *          3. Initialize display and UI
 *          4. Record start time for screen transitions
 *          5. Start BLE scanning (normal mode) OR web server (WiFi mode)
 *          6. Start LVGL UI system
 */
void Application::init() {
	// Set default log level for all components
	esp_log_level_set("*", ESP_LOG_WARN);
	esp_log_level_set("lv", ESP_LOG_WARN);

	// Mount SPIFFS filesystem for PNG images
	if (!SPIFFSManager::instance().init()) {
		ESP_LOGW("Application", "Failed to mount SPIFFS - PNG images may not load");
	}

	// Load configuration from NVS (sensors, brightness, WiFi mode flag)
	loadConfiguration();
	
	// Check if we should boot into WiFi configuration mode
	m_wifiConfigMode = isWiFiConfigMode();
	
	// Initialize LCD display and UI controllers
	initializeDisplay();
	
	// Record start timestamp for screen transition timing
	recordStartTime();
	
	if (!m_wifiConfigMode) {
		// Normal mode: Start BLE scanning for TPMS sensors
		initBLE();
	}
	
	// Start LVGL tick timer for UI updates
	startUISystem();
	
	if (m_wifiConfigMode) {
		// WiFi config mode: Start AP and web server for OTA/config
		startConfigServer();
	}
}

/**
 * @brief Load application configuration from NVS
 * @details Loads and validates:
 *          - Sensor addresses (front/rear)
 *          - Ideal pressure values (PSI/BAR)
 *          - Pressure unit preference
 *          - Display brightness level
 *          - Updates State pairing status
 */
void Application::loadConfiguration() {
	State &state = State::getInstance();

	// Initialize ConfigManager and load JSON from NVS
	m_config.init();

	// Load sensor MAC addresses
	std::string frontAddr, rearAddr;
	m_config.getString("front_address", frontAddr, "");
	m_config.getString("rear_address", rearAddr, "");
	state.setFrontAddress(frontAddr);
	state.setRearAddress(rearAddr);

	// Load ideal pressure values with defaults
	float frontPSI, rearPSI;
	m_config.getFloat("front_ideal_psi", frontPSI, DEFAULT_FRONT_PSI);
	m_config.getFloat("rear_ideal_psi", rearPSI, DEFAULT_REAR_PSI);
	state.setFrontIdealPSI(frontPSI);
	state.setRearIdealPSI(rearPSI);
	
	// Load pressure unit preference (PSI or BAR)
	std::string unit;
	m_config.getString("pressure_unit", unit, "PSI");
	state.setPressureUnit(unit);

	// Load and validate brightness setting (0-4 index into BRIGHTNESS_LEVELS array)
	int brightnessIndex = DEFAULT_BRIGHTNESS_INDEX;
	m_config.getInt("brightness_index", brightnessIndex, DEFAULT_BRIGHTNESS_INDEX);
	m_currentBrightnessIndex = static_cast<uint8_t>(brightnessIndex);
	if (m_currentBrightnessIndex > MAX_BRIGHTNESS_INDEX) {
		m_currentBrightnessIndex = MAX_BRIGHTNESS_INDEX;
	}

	// Update pairing status based on whether both addresses are configured
	state.setIsPaired(!state.getFrontAddress().empty() && !state.getRearAddress().empty());
}

/**
 * @brief Initialize display hardware and UI controllers
 * @details Steps:
 *          1. Get DisplayManager singleton and initialize LCD/LVGL
 *          2. Get UIController and PairController singletons
 *          3. Set version label or WiFi mode label on splash screen
 *          4. Apply saved brightness setting from configuration
 */
void Application::initializeDisplay() {
	// Initialize LCD display and LVGL
	m_display = DisplayManager::instance();
	m_display->init();

	// Get UI controller instances
	m_uiController = &UIController::instance();
	m_pairController = &PairController::instance();

	// Set version or WiFi mode label before any screen transitions
	if (m_wifiConfigMode) {
		m_uiController->setWiFiModeLabel();
	} else {
		m_uiController->setVersionLabel();
	}

	// Apply saved brightness setting from configuration
	m_display->setBacklightBrightness(BRIGHTNESS_LEVELS[m_currentBrightnessIndex]);
}

/**
 * @brief Record application start timestamp
 * @details Used for calculating elapsed time for screen transitions
 */
void Application::recordStartTime() {
	m_startTime = esp_timer_get_time() / 1000; // Convert microseconds to milliseconds
}

/**
 * @brief Start LVGL tick timer for UI updates
 */
void Application::startUISystem() {
	m_uiController->startLVGLTickTimer();
}

/**
 * @brief Start application main loop
 * @details Creates two FreeRTOS tasks:
 *          1. LVGL task - handles GUI rendering and touch input
 *          2. Control logic task - handles screen transitions, button input, app state
 */
void Application::run() {
	// Start LVGL timer handler task (handles GUI updates at ~30fps)
	m_uiController->startLVGLTask();

	// Create control logic task (handles screen transitions and app state)
	xTaskCreate(controlLogicTaskWrapper, "control_logic", 2048, this,
				tskIDLE_PRIORITY + 2, nullptr);
}

/**
 * @brief Initialize BLE subsystem for TPMS sensor scanning
 * @details Configures NimBLE with:
 *          - Active scanning (request scan responses)
 *          - 100ms interval, 50ms window (50% duty cycle)
 *          - WiFi coexistence friendly parameters
 *          - Continuous scanning mode
 */
void Application::initBLE() {
	// Initialize NimBLE stack
	NimBLEDevice::init("");
	NimBLEScan *pBLEScan = NimBLEDevice::getScan();

	// Set scan callbacks for TPMS sensor detection
	pBLEScan->setScanCallbacks(&m_scanCallbacks, false);
	
	// Active scan with moderate parameters for balance between speed and WiFi coexistence
	pBLEScan->setActiveScan(true);  // Request scan response packets
	pBLEScan->setInterval(100);     // 62.5ms scan interval
	pBLEScan->setWindow(50);        // 31.25ms scan window (50% duty cycle)
	pBLEScan->setMaxResults(0);     // No limit on results
	
	// Start continuous scanning
	pBLEScan->start(BLE_SCAN_TIME_MS, false, true);
}

/**
 * @brief Main control logic task - runs application state machine
 * @details Responsibilities:
 *          - Manage screen transitions (splash -> main/pair)
 *          - Handle button input for brightness/pairing/mode changes
 *          - Update pairing controller in pairing mode
 *          - Update UI with sensor data in normal mode
 * 
 * Operating modes:
 * - WiFi Config Mode: Show splash, wait for button press to exit
 * - Pairing Mode: Guide user through sensor pairing workflow
 * - Normal Mode: Display sensor data, handle brightness control
 */
void Application::controlLogicTask() {
	bool splashShown = false;
	bool mainShown = false;
	bool inPairingMode = false;

	// Configure GPIO9 as button input with pullup
	configureButton();

	// Initialize button state tracking
	ButtonState buttonState = {};

	// Main application loop
	for (;;) {
		uint32_t elapsed = getElapsedTime();
		uint32_t currentTime = esp_timer_get_time() / 1000;

		// In WiFi config mode, stay on splash screen and only handle button input
		if (m_wifiConfigMode) {
			// Show splash screen with WiFi mode label
			if (elapsed >= SPLASH_SCREEN_DELAY_MS && !splashShown) {
				lv_async_call(showSplashScreenCallback, nullptr);
				splashShown = true;
				printf("Showing splash screen (WiFi config mode)\\n");
			}
			// Monitor button for exit request (2s press)
			handleButtonInput(buttonState);
		} else {
			// Normal mode operation
			handleScreenTransitions(elapsed, splashShown, mainShown);

			if (mainShown) {
				State &state = State::getInstance();
				
				if (!state.getIsPaired()) {
					// In pairing mode: run pairing state machine
					if (!inPairingMode) {
						inPairingMode = true;
					}
					m_pairController->update(currentTime);
					handleButtonInput(buttonState);
				} else {
					// Normal operation: monitor sensors and handle button input
					handleButtonInput(buttonState);
					updateUIIfPaired();
				}
			}
		}

		// Run control loop at 10Hz
		vTaskDelay(pdMS_TO_TICKS(CONTROL_LOOP_DELAY_MS));
	}
}

/**
 * @brief Configure GPIO9 as button input with pullup
 * @details Sets up button with:
 *          - Input mode
 *          - Internal pullup enabled
 *          - No interrupts (polled in control loop)
 */
void Application::configureButton() {
	gpio_config_t io_conf = {};
	io_conf.pin_bit_mask = (1ULL << GPIO_NUM_9);
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&io_conf);
}

/**
 * @brief Get elapsed time since application start
 * @return Milliseconds since init()
 */
uint32_t Application::getElapsedTime() const {
	return (esp_timer_get_time() / 1000) - m_startTime;
}

/**
 * @brief Handle timed screen transitions at startup
 * @param elapsed Time since application start (milliseconds)
 * @param splashShown Flag tracking if splash screen has been shown
 * @param mainShown Flag tracking if main/pair screen has been shown
 * 
 * @details Transition flow:
 *          1. At 1000ms: Show splash screen with logo/version
 *          2. At 4500ms: Show main screen (if paired) OR pair screen (if not paired)
 *          3. Initialize sensor labels if showing main screen
 */
void Application::handleScreenTransitions(uint32_t elapsed, bool &splashShown,
										  bool &mainShown) {
	State &state = State::getInstance();
	
	// Step 1: Show splash screen after 1 second
	if (elapsed >= SPLASH_SCREEN_DELAY_MS && !splashShown) {
		lv_async_call(showSplashScreenCallback, nullptr);
		splashShown = true;
	} 
	// Step 2: Show main/pair screen after 4.5 seconds
	else if (elapsed >= MAIN_SCREEN_DELAY_MS && !mainShown) {
		if (state.getIsPaired()) {
			// Sensors are paired: Show main screen with sensor data
			lv_async_call(initializeLabelsCallback, nullptr);
			vTaskDelay(pdMS_TO_TICKS(LABEL_INIT_DELAY_MS));
			lv_async_call(showMainScreenCallback, nullptr);
		} else {
			// Sensors not paired: Show pairing workflow screen
			lv_async_call(showPairScreenCallback, nullptr);
			m_pairController->init();
		}
		mainShown = true;
	}
}

/**
 * @brief Handle button press/release events and duration tracking
 * @param state ButtonState struct tracking press state and timing
 * 
 * @details Button press durations:
 *          - Short press (<2s): Cycle brightness (normal mode) or pairing action
 *          - Long press (2-15s): Clear sensor addresses and reboot
 *          - Very long press (>15s): Enter WiFi configuration mode
 *          - WiFi mode long press (>2s): Exit WiFi mode and reboot
 * 
 * Implementation notes:
 * - Button is active-low (pressed = GPIO low)
 * - Very long press has priority over long press
 * - Uses state machine to prevent double-handling
 */
void Application::handleButtonInput(ButtonState &state) {
	bool currentButtonState = gpio_get_level(GPIO_NUM_9);
	uint32_t currentTime = esp_timer_get_time() / 1000;

	// Button press detected (transition from high to low)
	if (state.lastState && !currentButtonState) {
		state.pressStartTime = currentTime;
		state.pressHandled = false;
	} 
	// Button held down - check duration for long/very long press
	else if (!currentButtonState && !state.pressHandled) {
		uint32_t pressDuration = currentTime - state.pressStartTime;
		
		if (m_wifiConfigMode) {
			// In WiFi mode: Any long press exits config mode
			if (pressDuration >= LONG_PRESS_DURATION_MS) {
				exitWiFiConfigMode();
				state.pressHandled = true;
			}
		} else {
			// Normal mode: Check for very long press first, then long press
			if (pressDuration >= VERY_LONG_PRESS_DURATION_MS) {
				handleVeryLongPress();
				state.pressHandled = true;
			} else if (pressDuration >= LONG_PRESS_DURATION_MS && pressDuration < VERY_LONG_PRESS_DURATION_MS) {
				// Don't handle long press yet - wait to see if user continues to very long press
			}
		}
	} 
	// Button released - determine action based on press duration
	else if (!state.lastState && currentButtonState) {
		uint32_t pressDuration = currentTime - state.pressStartTime;
		if (!state.pressHandled) {
			if (pressDuration >= LONG_PRESS_DURATION_MS && pressDuration < VERY_LONG_PRESS_DURATION_MS) {
				// Released after long press but before very long press
				handleLongPress();
			} else if (pressDuration < LONG_PRESS_DURATION_MS) {
				// Short press
				handleShortPress();
			}
		}
		state.pressHandled = false;
	}

	// Update state for next iteration
	state.lastState = currentButtonState;
}

/**
 * @brief Handle long button press (2-15 seconds)
 * @details Clears sensor pairing addresses from configuration and reboots device
 *          to restart the pairing process from scratch
 */
void Application::handleLongPress() {
	// Clear sensor addresses from configuration
	m_config.setString("front_address", "");
	m_config.setString("rear_address", "");
	
	// Brief delay for user feedback
	vTaskDelay(pdMS_TO_TICKS(500));
	
	// Reboot device to re-enter pairing mode
	esp_restart();
}

/**
 * @brief Handle very long button press (>15 seconds)
 * @details Enters WiFi configuration mode for OTA updates and remote configuration
 */
void Application::handleVeryLongPress() {
	enterWiFiConfigMode();
}

/**
 * @brief Handle short button press (<2 seconds)
 * @details Action depends on current mode:
 *          - Pairing mode: Pass press to PairController for pairing workflow
 *          - Normal mode: Cycle through brightness levels (10%, 30%, 50%, 75%, 100%)
 */
void Application::handleShortPress() {
	State &state = State::getInstance();
	
	if (!state.getIsPaired()) {
		// In pairing mode: Let PairController handle button press
		m_pairController->handleButtonPress();
	} else {
		// Normal mode: Cycle display brightness
		cycleBrightness();
	}
}

/**
 * @brief Cycle through display brightness levels
 * @details Cycles through 5 levels (10%, 30%, 50%, 75%, 100%) and saves preference
 */
void Application::cycleBrightness() {
	// Cycle to next brightness level (wraps at 5)
	m_currentBrightnessIndex = (m_currentBrightnessIndex + 1) % 5;
	uint8_t brightness = BRIGHTNESS_LEVELS[m_currentBrightnessIndex];
	
	// Apply new brightness to display
	m_display->setBacklightBrightness(brightness);
	
	// Save brightness preference to NVS
	m_config.setInt("brightness_index", m_currentBrightnessIndex);
}

/**
 * @brief Update UI with sensor data if sensors are paired
 * @details Requests async UI update in LVGL task context
 */
void Application::updateUIIfPaired() {
	State &state = State::getInstance();
	if (state.getIsPaired()) {
		lv_async_call(updateLabelsCallback, nullptr);
	}
}

// ============================================================================
// LVGL Async Callbacks
// ============================================================================
// These callbacks are invoked via lv_async_call() to ensure they run in the
// LVGL task context. This is required for thread-safe GUI updates.

/**
 * @brief Callback to set version label on splash screen
 * @param arg Unused parameter (required by lv_async_call signature)
 */
void Application::setVersionLabelCallback(void *arg) {
	(void)arg;
	UIController::instance().setVersionLabel();
}

/**
 * @brief Callback to show splash screen
 * @param arg Unused parameter (required by lv_async_call signature)
 */
void Application::showSplashScreenCallback(void *arg) {
	(void)arg;
	UIController::instance().showSplashScreen();
}

/**
 * @brief Callback to show main sensor data screen
 * @param arg Unused parameter (required by lv_async_call signature)
 */
void Application::showMainScreenCallback(void *arg) {
	(void)arg;
	UIController::instance().showMainScreen();
}

/**
 * @brief Callback to show sensor pairing screen
 * @param arg Unused parameter (required by lv_async_call signature)
 */
void Application::showPairScreenCallback(void *arg) {
	(void)arg;
	UIController::instance().showPairScreen();
}

/**
 * @brief Callback to initialize sensor data labels
 * @param arg Unused parameter (required by lv_async_call signature)
 */
void Application::initializeLabelsCallback(void *arg) {
	(void)arg;
	UIController::instance().initializeLabels();
}

/**
 * @brief Callback to update sensor data labels on main screen
 * @param arg Unused parameter (required by lv_async_call signature)
 * @details Retrieves sensor data from State, updates alert blink state,
 *          and calls UIController to refresh all sensor displays
 */
void Application::updateLabelsCallback(void *arg) {
	(void)arg;
	State &state = State::getInstance();
	UIController &ui = UIController::instance();

	// Look up sensor data by address
	TPMSUtil *frontSensor = nullptr;
	TPMSUtil *rearSensor = nullptr;

	auto frontIt = state.getData().find(state.getFrontAddress());
	if (frontIt != state.getData().end()) {
		frontSensor = frontIt->second;
	}

	auto rearIt = state.getData().find(state.getRearAddress());
	if (rearIt != state.getData().end()) {
		rearSensor = rearIt->second;
	}

	// Update alert blink state for warning indicators
	uint32_t currentTime = esp_timer_get_time() / 1000;
	ui.updateAlertBlinkState(currentTime);

	// Update UI with current sensor readings
	ui.updateSensorUI(frontSensor, rearSensor, state.getFrontIdealPSI(),
					  state.getRearIdealPSI(), currentTime);
}

/**
 * @brief FreeRTOS task wrapper for controlLogicTask
 * @param pvParameter Pointer to Application instance
 */
void Application::controlLogicTaskWrapper(void *pvParameter) {
	static_cast<Application *>(pvParameter)->controlLogicTask();
}

// ============================================================================
// WiFi Configuration Mode
// ============================================================================

/**
 * @brief Check if device should boot into WiFi configuration mode
 * @return true if wifi_config_mode flag is set in NVS
 */
bool Application::isWiFiConfigMode() {
	int wifiMode = 0;
	m_config.getInt("wifi_config_mode", wifiMode, 0);
	return wifiMode == 1;
}

/**
 * @brief Enter WiFi configuration mode
 * @details Sets wifi_config_mode flag in NVS and reboots device
 *          into AP mode with web server for OTA updates
 */
void Application::enterWiFiConfigMode() {
	m_config.setInt("wifi_config_mode", 1);
	vTaskDelay(pdMS_TO_TICKS(500));
	esp_restart();
}

/**
 * @brief Exit WiFi configuration mode
 * @details Clears wifi_config_mode flag in NVS and reboots device
 *          back to normal TPMS monitoring mode
 */
void Application::exitWiFiConfigMode() {
	m_config.setInt("wifi_config_mode", 0);
	vTaskDelay(pdMS_TO_TICKS(500));
	esp_restart();
}

/**
 * @brief Start WiFi AP and web server for configuration mode
 * @details Starts:
 *          1. WiFi Access Point (SSID: TPMS-Config)
 *          2. HTTP web server for OTA updates and config API
 * 
 * If initialization fails, cleans up and returns without crashing
 */
void Application::startConfigServer() {
	// Initialize and start WiFi Access Point
	WiFiManager &wifi = WiFiManager::instance();
	if (!wifi.init()) {
		return;
	}

	if (!wifi.start()) {
		return;
	}

	// Start HTTP web server
	WebServer &web = WebServer::instance();
	if (!web.start()) {
		wifi.stop();  // Clean up WiFi if server fails
		return;
	}
}