/**
 * @file PairController.cpp
 * @brief Implementation of sensor pairing workflow state machine
 */

#include "PairController.h"
#include "Application.h"       // Application instance
#include "State.h"              // Global state
#include "WiFiManager.h"        // WiFi AP management
#include "WebServer.h"          // HTTP server
#include "UI/ui.h"              // LVGL UI elements
#include "esp_timer.h"          // High-resolution timer
#include "esp_log.h"            // ESP logging
#include "lvgl.h"               // LVGL UI functions
#include <NimBLEDevice.h>       // BLE scanning

/// Log tag for PairController module
static const char* TAG = "PairController";

/**
 * @brief Get singleton instance (Meyer's singleton)
 * @return Reference to the PairController singleton
 */
PairController &PairController::instance() {
	static PairController controller;
	return controller;
}

/**
 * @brief Initialize pairing mode
 * @details Preparation steps:
 *          1. Reset pairing state variables
 *          2. Stop WiFi and web server for better BLE performance
 *          3. Switch to aggressive active BLE scanning (99% duty cycle)
 *          4. Display initial UI (waiting for button press)
 */
void PairController::init() {
	ESP_LOGI(TAG, "Initializing pairing mode");
	
	// Reset pairing state
	State &state = State::getInstance();
	m_expectedCount = (state.getMode() == MODE_CAR) ? 4 : 2;
	m_selectedAddresses.clear();
	m_selectedAddresses.resize(m_expectedCount);
	m_currentIndex = 0;
	m_state = PairingState::SCANNING_INDEX;
	m_pairingComplete = false;
	m_scanStartTime = 0;  // Will be set when user presses button to start
	
	// Stop WiFi and WebServer during pairing for better BLE performance
	ESP_LOGI(TAG, "Stopping WiFi for better BLE scanning");
	WebServer::instance().stop();
	WiFiManager::instance().stop();
	
	// Switch to aggressive active BLE scan for pairing
	NimBLEScan *pBLEScan = NimBLEDevice::getScan();
	pBLEScan->stop();
	pBLEScan->setActiveScan(true);  // Request scan response packets
	pBLEScan->setInterval(100);     // 62.5ms interval
	pBLEScan->setWindow(99);        // ~62ms window (99% duty cycle - very aggressive)
	pBLEScan->start(0, false, false);  // Continuous scan
	ESP_LOGI(TAG, "Switched to active BLE scan");
	
	// Show initial UI - waiting for button press to start
	lv_async_call(initUICallback, this);
}

/**
 * @brief Async callback to initialize pairing UI (thread-safe)
 * @param arg Pointer to PairController instance
 */
void PairController::initUICallback(void *arg) {
	PairController *controller = static_cast<PairController *>(arg);
	(void)controller;
	// Display label based on mode/index
	State &state = State::getInstance();
	std::string label;
	if (state.getMode() == MODE_BIKE) {
		label = std::string(SENSOR_BIKE_FRONT_TEXT);
	} else {
		label = std::string(SENSOR_CAR_FRONT_LEFT_TEXT);
	}
	lv_label_set_text(ui_PairSensorName, label.c_str());
	lv_label_set_text(ui_Status, "START PAIRING");
	lv_obj_set_style_text_color(ui_Status, lv_color_hex(0xFFFF00), LV_PART_MAIN);  // Yellow
	lv_obj_add_flag(ui_PairBusy, LV_OBJ_FLAG_HIDDEN);    // Hide spinner
	lv_obj_clear_flag(ui_PressKey, LV_OBJ_FLAG_HIDDEN);   // Show button icon
	lv_label_set_text(ui_PairTimeout, "---");
}

/**
 * @brief Start front wheel sensor scanning
 * @details Resets scan timer and UI, starts 60-second countdown
 */
void PairController::startFrontScan() {
	// Convenience wrapper - start scan for index 0
	startIndexScan(0);
}

void PairController::startIndexScan(int index) {
	State &state = State::getInstance();
	m_currentIndex = index;
	ESP_LOGI(TAG, "Starting scan for index %d", index);
	m_state = PairingState::SCANNING_INDEX;
	m_scanStartTime = esp_timer_get_time() / 1000;
	m_lastSensorCount = 0;

	// Prepare label for UI based on index and mode
	std::string label;
	if (state.getMode() == MODE_BIKE) {
		if (index == SENSOR_BIKE_FRONT) label = SENSOR_BIKE_FRONT_TEXT;
		else if (index == SENSOR_BIKE_REAR) label = SENSOR_BIKE_REAR_TEXT;
	} else {
		switch (index) {
			case 0: label = SENSOR_CAR_FRONT_LEFT_TEXT; break;
			case 1: label = SENSOR_CAR_FRONT_RIGHT_TEXT; break;
			case 2: label = SENSOR_CAR_REAR_LEFT_TEXT; break;
			case 3: label = SENSOR_CAR_REAR_RIGHT_TEXT; break;
			default: label = "SENSOR"; break;
		}
	}
	// Update UI label text via async callback
	lv_label_set_text(ui_PairSensorName, label.c_str());
	lv_async_call(startScanningUICallback, this);
}

/**
 * @brief Async callback to start front scan UI (thread-safe)
 * @param arg Pointer to PairController instance
 */
void PairController::startFrontScanUICallback(void *arg) {
	PairController *controller = static_cast<PairController *>(arg);
	(void)controller;
	// Will be set by startIndexScan to set proper label
	lv_label_set_text(ui_Status, "START PAIRING");
	lv_obj_set_style_text_color(ui_Status, lv_color_hex(0xFFFF00), LV_PART_MAIN);  // Yellow
	lv_obj_add_flag(ui_PairBusy, LV_OBJ_FLAG_HIDDEN);    // Hide spinner
	lv_obj_clear_flag(ui_PressKey, LV_OBJ_FLAG_HIDDEN);   // Show button icon
	lv_label_set_text(ui_PairTimeout, "60s");                // Show initial timeout
}

/**
 * @brief Start rear wheel sensor scanning
 * @details Resets scan timer and UI, starts 60-second countdown
 */
void PairController::startRearScan() {
	// Convenience wrapper - start scan for index 1 (rear) or next index
	startIndexScan(m_currentIndex);
}

/**
 * @brief Async callback to start rear scan UI (thread-safe)
 * @param arg Pointer to PairController instance
 */
void PairController::startRearScanUICallback(void *arg) {
	PairController *controller = static_cast<PairController *>(arg);
	(void)controller;
	// Label is set dynamically based on index
	lv_label_set_text(ui_Status, "START PAIRING");
	lv_obj_set_style_text_color(ui_Status, lv_color_hex(0xFFFF00), LV_PART_MAIN);  // Yellow
	lv_obj_add_flag(ui_PairBusy, LV_OBJ_FLAG_HIDDEN);    // Hide spinner
	lv_obj_clear_flag(ui_PressKey, LV_OBJ_FLAG_HIDDEN);   // Show button icon
	lv_label_set_text(ui_PairTimeout, "60s");                // Show initial timeout
}

/**
 * @brief Update pairing state machine
 * @param currentTime Current timestamp in milliseconds
 * @details Periodically called from main control loop.\n *          Responsibilities:\n *          - Check for new sensors during scanning\n *          - Update timeout countdown display (60s -> 0s)\n *          - Handle scan timeout and transition to TIMEOUT state\n */
void PairController::update(uint32_t currentTime) {
	// Skip if pairing is complete
	if (m_state == PairingState::COMPLETE) {
		return;
	}

	// Check for new sensors
	checkForNewSensor();

	// Update timeout counter (only if scan has started)
	if (m_state == PairingState::SCANNING_INDEX && m_scanStartTime > 0) {
		uint32_t elapsed = currentTime - m_scanStartTime;
		uint32_t remaining = (SCAN_TIMEOUT_MS - elapsed) / 1000;  // Convert to seconds
		
		// Update countdown display
		if (elapsed < SCAN_TIMEOUT_MS) {
			char timeoutText[16];
			snprintf(timeoutText, sizeof(timeoutText), "%lus", remaining);
			// Use async call for UI update
			m_pendingTimeoutText = timeoutText;
			lv_async_call(updateTimeoutCallback, this);
		} else {
			// Timeout reached - show message and wait for button press to retry
			ESP_LOGW(TAG, "Scan timeout");
			
			// Transition to timeout state
			m_state = PairingState::TIMEOUT_INDEX;
			
			// Update UI to show timeout
			lv_async_call(timeoutUICallback, this);
			m_scanStartTime = 0;  // Reset for next attempt
		}
	}
}

/**
 * @brief Check for new TPMS sensors during scanning
 * @details Monitors State sensor map and detects when a new sensor appears.\n *          Front sensor: Selects first detected sensor\n *          Rear sensor: Selects first detected sensor different from front\n */
void PairController::checkForNewSensor() {
	// Only check during active scanning states
	if (m_state != PairingState::SCANNING_INDEX) {
		return;
	}

	State &state = State::getInstance();
	uint32_t currentCount = state.getData().size();

	// Check if a new sensor appeared
	if (currentCount > m_lastSensorCount && currentCount > 0) {
		// Get the newest sensor address (first in map)
		std::string newestAddress;
		for (const auto &pair : state.getData()) {
			newestAddress = pair.first;
			break; // Take first one for now
		}

		// Assign to current index and ensure it's unique
		bool alreadySelected = false;
		for (const auto &addr : m_selectedAddresses) {
			if (!addr.empty() && addr == newestAddress) {
				alreadySelected = true;
				break;
			}
		}
		if (!alreadySelected) {
			m_selectedAddresses[m_currentIndex] = newestAddress;
			m_state = PairingState::WAITING_INDEX_CONFIRM;
			ESP_LOGI(TAG, "Sensor found for index %d: %s", m_currentIndex, newestAddress.c_str());
			updateUI();
		} else {
			ESP_LOGW(TAG, "Ignoring sensor - already selected: %s", newestAddress.c_str());
		}
	}

	// Update sensor count for next iteration
	m_lastSensorCount = currentCount;
}

/**
 * @brief Update UI based on current pairing state
 * @details Updates label colors, text, spinner, and button icon visibility
 *          Green text: Sensor found, waiting for confirmation
 *          Yellow text: Waiting to start scan or timeout
 *          White text: Scanning in progress
 */
void PairController::updateUI() {
	lv_async_call(updateUICallback, this);
}

/**
 * @brief Async callback to update timeout text (thread-safe)
 * @param arg Pointer to PairController instance
 */
void PairController::updateTimeoutCallback(void *arg) {
	PairController *controller = static_cast<PairController *>(arg);
	lv_label_set_text(ui_PairTimeout, controller->m_pendingTimeoutText.c_str());
}

/**
 * @brief Async callback to show timeout UI (thread-safe)
 * @param arg Pointer to PairController instance
 */
void PairController::timeoutUICallback(void *arg) {
	PairController *controller = static_cast<PairController *>(arg);
	(void)controller;
	lv_label_set_text(ui_Status, "TIMEOUT");
	lv_obj_set_style_text_color(ui_Status, lv_color_hex(0xFFFF00), LV_PART_MAIN);  // Yellow
	lv_obj_add_flag(ui_PairBusy, LV_OBJ_FLAG_HIDDEN);    // Hide spinner
	lv_obj_clear_flag(ui_PressKey, LV_OBJ_FLAG_HIDDEN);   // Show button icon
	lv_label_set_text(ui_PairTimeout, "0s");
}

/**
 * @brief Async callback to show scanning UI (thread-safe)
 * @param arg Pointer to PairController instance
 */
void PairController::startScanningUICallback(void *arg) {
	PairController *controller = static_cast<PairController *>(arg);
	(void)controller;
	lv_label_set_text(ui_Status, "SCANNING...");
	lv_obj_set_style_text_color(ui_Status, lv_color_hex(0xFFFFFF), LV_PART_MAIN);  // White
	lv_obj_clear_flag(ui_PairBusy, LV_OBJ_FLAG_HIDDEN);  // Show spinner
	lv_obj_add_flag(ui_PressKey, LV_OBJ_FLAG_HIDDEN);     // Hide button icon
}

/**
 * @brief Async callback to update UI for sensor confirmation (thread-safe)
 * @param arg Pointer to PairController instance
 */
void PairController::updateUICallback(void *arg) {
	PairController *controller = static_cast<PairController *>(arg);
	
	if (controller->m_state == PairingState::WAITING_INDEX_CONFIRM) {
		const std::string &addr = controller->m_selectedAddresses[controller->m_currentIndex];
		lv_label_set_text(ui_PairSensorName, addr.c_str());
		// Display a generic message with index (front/rear mapping done by label text)
		lv_label_set_text(ui_Status, "CONFIRM SENSOR");
		lv_obj_set_style_text_color(ui_Status, lv_color_hex(0x00FF00), LV_PART_MAIN);  // Green
		lv_obj_add_flag(ui_PairBusy, LV_OBJ_FLAG_HIDDEN);    // Hide spinner
		lv_obj_clear_flag(ui_PressKey, LV_OBJ_FLAG_HIDDEN);   // Show button icon
	}
}

/**
 * @brief Async callback to show pairing complete UI (thread-safe)
 * @param arg Pointer to PairController instance
 */
void PairController::pairingCompleteUICallback(void *arg) {
	PairController *controller = static_cast<PairController *>(arg);
	(void)controller;
	lv_label_set_text(ui_Status, "PAIRING COMPLETE");
	lv_obj_add_flag(ui_PressKey, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief Handle button press events during pairing
 * @details Button actions by state:
 *          - SCANNING/TIMEOUT: (Re)start scan with timeout
 *          - WAITING_FRONT_CONFIRM: Confirm front sensor, move to rear scan
 *          - WAITING_REAR_CONFIRM: Confirm rear sensor, save and reboot
 */
void PairController::handleButtonPress() {
	ESP_LOGD(TAG, "Button pressed in state %d", static_cast<int>(m_state));

	if (m_state == PairingState::SCANNING_INDEX || m_state == PairingState::TIMEOUT_INDEX) {
		// Start or retry front sensor scan
		ESP_LOGI(TAG, "Starting/retrying front sensor scan");
		lv_async_call(startScanningUICallback, this);
		m_state = PairingState::SCANNING_INDEX;
		m_scanStartTime = esp_timer_get_time() / 1000;  // Start timeout
	} else if (m_state == PairingState::WAITING_INDEX_CONFIRM) {
		// Confirmed sensor at index - either move to next index or complete
		ESP_LOGI(TAG, "Sensor at index %d confirmed", m_currentIndex);
		if (++m_currentIndex < m_expectedCount) {
			// Start scanning next index
			startIndexScan(m_currentIndex);
		} else {
			// All sensors confirmed
			ESP_LOGI(TAG, "All sensors confirmed, saving and rebooting");
			savePairingAndReboot();
		}
	}
}

/**
 * @brief Save paired sensor addresses to NVS and reboot
 * @details Steps:
 *          1. Validate both addresses are present
 *          2. Save addresses to ConfigManager (NVS)
 *          3. Update State with pairing status
 *          4. Restore normal BLE scan parameters
 *          5. Show completion message
 *          6. Reboot device to normal mode
 */
void PairController::savePairingAndReboot() {
	// Validate that we have selected addresses for all indexes
	for (int i = 0; i < m_expectedCount; ++i) {
		if (m_selectedAddresses[i].empty()) {
			ESP_LOGE(TAG, "Error - missing sensor address for index %d", i);
			return;
		}
	}

	// Log all selected addresses
	std::string joined;
	for (size_t i = 0; i < m_selectedAddresses.size(); ++i) {
		if (i) joined += ", ";
		joined += m_selectedAddresses[i];
	}
	ESP_LOGI(TAG, "Saving pairing - Addresses: %s", joined.c_str());

	// Save addresses to NVS via ConfigManager
	Application &app = Application::instance();
	ConfigManager &config = app.getConfig();
	State &state = State::getInstance();

	if (state.getMode() == MODE_BIKE) {
		config.setString("sensor_address_0", m_selectedAddresses[0]);
		config.setString("sensor_address_1", m_selectedAddresses[1]);
		state.setAddress(SENSOR_BIKE_FRONT, m_selectedAddresses[0]);
		state.setAddress(SENSOR_BIKE_REAR, m_selectedAddresses[1]);
	} else {
		// Car mode - write keys sensor_address_1..4 and update state
		for (int i = 0; i < 4; ++i) {
			std::string key = "sensor_address_" + std::to_string(i + 1);
			config.setString(key, m_selectedAddresses[i]);
			state.setAddress(i, m_selectedAddresses[i]);
		}
	}
	state.setIsPaired(true);

	// Update pairing controller state
	m_state = PairingState::COMPLETE;
	m_pairingComplete = true;

	// Show completion message
	lv_async_call(pairingCompleteUICallback, this);

	// Restore normal BLE scan parameters (WiFi coexistence friendly)
	ESP_LOGI(TAG, "Restoring normal BLE scan");
	NimBLEScan *pBLEScan = NimBLEDevice::getScan();
	pBLEScan->stop();
	pBLEScan->setActiveScan(true);
	pBLEScan->setInterval(100);  // 62.5ms interval
	pBLEScan->setWindow(50);     // 31.25ms window (50% duty cycle)
	pBLEScan->start(0, false, false);  // Continuous scan

	// Brief delay for user to see completion message, then reboot
	vTaskDelay(pdMS_TO_TICKS(1500));
	esp_restart();
}
