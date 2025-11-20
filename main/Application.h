#pragma once

#include "ConfigManager.h"
#include "DisplayManager.h"
#include "PairController.h"
#include "TPMSScanCallbacks.h"
#include "UIController.h"
#include "WiFiManager.h"
#include "WebServer.h"
#include <cstdint>

/**
 * @class Application
 * @brief Main application controller - singleton orchestrating all subsystems
 * @details Manages the complete lifecycle of the bike pressure monitor application:
 *          - Configuration loading/saving
 *          - Display initialization and UI control
 *          - BLE scanning for TPMS sensors
 *          - Sensor pairing workflow
 *          - Button input handling
 *          - WiFi configuration mode
 *          - Screen transitions and timing
 * 
 * Operating Modes:
 * - Normal Mode: BLE scanning, pressure monitoring, sensor display
 * - Pairing Mode: Guide user through sensor pairing process
 * - WiFi Config Mode: Web server for OTA updates and configuration
 */
class Application {
public:
#ifdef GIT_VERSION
	static constexpr char appVersion[] = GIT_VERSION;  ///< Version from git tag
#else
	static constexpr char appVersion[] = "1.0.0-dev";  ///< Development version
#endif

	/**
	 * @brief Get singleton instance
	 * @return Reference to the Application singleton
	 */
	static Application &instance();

	/**
	 * @brief Initialize all application subsystems
	 * @details Loads configuration, initializes display, sets up BLE or WiFi mode
	 */
	void init();
	
	/**
	 * @brief Start application main loop
	 * @details Creates FreeRTOS tasks for UI updates and control logic
	 */
	void run();

	/**
	 * @brief Get configuration manager
	 * @return Reference to ConfigManager
	 */
	ConfigManager &getConfig() { return m_config; }
	
	/**
	 * @brief Get application start timestamp
	 * @return Start time in milliseconds since boot
	 */
	uint32_t getStartTime() const { return m_startTime; }

private:
	Application() = default;   ///< Private constructor for singleton
	~Application() = default;  ///< Private destructor

	Application(const Application &) = delete;             ///< No copy constructor
	Application &operator=(const Application &) = delete;  ///< No copy assignment

	// Initialization helpers
	void loadConfiguration();    ///< Load config from NVS (sensors, brightness, etc.)
	void initializeDisplay();    ///< Initialize LCD, LVGL, and UI controllers
	void recordStartTime();      ///< Record boot timestamp for screen timing
	void startUISystem();        ///< Start LVGL tick timer
	void initBLE();              ///< Initialize BLE scanning for TPMS sensors
	void startConfigServer();    ///< Start WiFi AP and web server for config mode

	// Main control task
	void controlLogicTask();  ///< Main application loop (screen transitions, button handling)

	// Control task helpers
	void configureButton();  ///< Configure GPIO9 as button input with pullup
	uint32_t getElapsedTime() const;  ///< Get milliseconds since application start
	void handleScreenTransitions(uint32_t elapsed, bool &splashShown,
								 bool &mainShown);  ///< Manage splash -> main/pair screen flow

	/**
	 * @struct ButtonState
	 * @brief Tracks button press state for debouncing and duration detection
	 */
	struct ButtonState {
		bool lastState = true;       ///< Previous button state (true = released)
		uint32_t pressStartTime = 0; ///< Timestamp when button was pressed
		bool pressHandled = false;   ///< Flag to prevent double-handling
		bool debounceActive = false; ///< Debounce timer active flag
		uint32_t debounceTime = 0;   ///< Debounce timeout timestamp
	};

	void handleButtonInput(ButtonState &state);  ///< Process button press/release events
	void handleLongPress();      ///< 2s press: Clear sensor pairing and reboot
	void handleVeryLongPress();  ///< 15s press: Enter WiFi config mode
	void handleShortPress();     ///< Short press: Cycle brightness or pairing action
	void cycleBrightness();      ///< Cycle through 5 brightness levels (10-100%)
	void updateUIIfPaired();     ///< Refresh sensor data on main screen
	
	// WiFi config mode helpers
	bool isWiFiConfigMode();     ///< Check if wifi_config_mode flag is set in NVS
	void enterWiFiConfigMode();  ///< Set flag and reboot into WiFi mode
	void exitWiFiConfigMode();   ///< Clear flag and reboot into normal mode

	// LVGL async callbacks (must run in LVGL task context)
	static void setVersionLabelCallback(void *arg);      ///< Set version text on splash screen
	static void showSplashScreenCallback(void *arg);     ///< Show splash screen
	static void showMainScreenCallback(void *arg);       ///< Show main sensor screen
	static void showPairScreenCallback(void *arg);       ///< Show sensor pairing screen
	static void initializeLabelsCallback(void *arg);     ///< Initialize sensor labels
	static void updateLabelsCallback(void *arg);         ///< Update sensor data on main screen

	// FreeRTOS task wrapper
	static void controlLogicTaskWrapper(void *pvParameter);  ///< Static wrapper for task creation

	// Member variables
	ConfigManager m_config;                 ///< Configuration manager (NVS persistence)
	DisplayManager *m_display = nullptr;    ///< Display manager (LCD/LVGL)
	UIController *m_uiController = nullptr; ///< UI controller (screen/label updates)
	PairController *m_pairController = nullptr; ///< Sensor pairing state machine
	TPMSScanCallbacks m_scanCallbacks;      ///< BLE scan callbacks for TPMS detection
	uint32_t m_startTime = 0;               ///< Application start timestamp (ms)

	static constexpr uint8_t BRIGHTNESS_LEVELS[5] = {10, 30, 50, 75, 100}; ///< Available brightness percentages
	uint8_t m_currentBrightnessIndex = 4;   ///< Current brightness level index (0-4)
	bool m_wifiConfigMode = false;          ///< True if in WiFi configuration mode
};