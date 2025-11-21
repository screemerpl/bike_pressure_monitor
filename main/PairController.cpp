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
	m_state = PairingState::SCANNING_FRONT;
	m_selectedFrontAddress.clear();
	m_selectedRearAddress.clear();
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
	lv_label_set_text(ui_Label10, "-FRONT WHEEL-");
	lv_label_set_text(ui_Label11, "START PAIRING");
	lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0xFFFF00), LV_PART_MAIN);  // Yellow
	lv_obj_add_flag(ui_Spinner4, LV_OBJ_FLAG_HIDDEN);    // Hide spinner
	lv_obj_clear_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);   // Show button icon
	lv_label_set_text(ui_Label12, "---");
}

/**
 * @brief Start front wheel sensor scanning
 * @details Resets scan timer and UI, starts 60-second countdown
 */
void PairController::startFrontScan() {
	ESP_LOGI(TAG, "Starting front wheel scan");
	
	m_state = PairingState::SCANNING_FRONT;
	m_scanStartTime = esp_timer_get_time() / 1000;  // Current time in ms
	m_lastSensorCount = 0;
	
	// Update UI for front wheel scan
	lv_label_set_text(ui_Label10, "-FRONT WHEEL-");
	lv_label_set_text(ui_Label11, "START PAIRING");
	lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0xFFFF00), LV_PART_MAIN);  // Yellow
	lv_obj_add_flag(ui_Spinner4, LV_OBJ_FLAG_HIDDEN);    // Hide spinner
	lv_obj_clear_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);   // Show button icon
	lv_label_set_text(ui_Label12, "60s");                // Show initial timeout
}

/**
 * @brief Start rear wheel sensor scanning
 * @details Resets scan timer and UI, starts 60-second countdown
 */
void PairController::startRearScan() {
	ESP_LOGI(TAG, "Starting rear wheel scan");
	
	m_state = PairingState::SCANNING_REAR;
	m_scanStartTime = esp_timer_get_time() / 1000;  // Current time in ms
	m_lastSensorCount = 0;
	
	// Update UI for rear wheel scan
	lv_label_set_text(ui_Label10, "-REAR WHEEL-");
	lv_label_set_text(ui_Label11, "START PAIRING");
	lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0xFFFF00), LV_PART_MAIN);  // Yellow
	lv_obj_add_flag(ui_Spinner4, LV_OBJ_FLAG_HIDDEN);    // Hide spinner
	lv_obj_clear_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);   // Show button icon
	lv_label_set_text(ui_Label12, "60s");                // Show initial timeout
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
	if ((m_state == PairingState::SCANNING_FRONT || m_state == PairingState::SCANNING_REAR) && m_scanStartTime > 0) {
		uint32_t elapsed = currentTime - m_scanStartTime;
		uint32_t remaining = (SCAN_TIMEOUT_MS - elapsed) / 1000;  // Convert to seconds
		
		// Update countdown display
		if (elapsed < SCAN_TIMEOUT_MS) {
			char timeoutText[16];
			snprintf(timeoutText, sizeof(timeoutText), "%lus", remaining);
			lv_label_set_text(ui_Label12, timeoutText);
		} else {
			// Timeout reached - show message and wait for button press to retry
			ESP_LOGW(TAG, "Scan timeout");
			
			// Transition to timeout state
			if (m_state == PairingState::SCANNING_FRONT) {
				m_state = PairingState::TIMEOUT_FRONT;
			} else {
				m_state = PairingState::TIMEOUT_REAR;
			}
			
			// Update UI to show timeout
			lv_label_set_text(ui_Label11, "TIMEOUT");
			lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0xFFFF00), LV_PART_MAIN);  // Yellow
			lv_obj_add_flag(ui_Spinner4, LV_OBJ_FLAG_HIDDEN);    // Hide spinner
			lv_obj_clear_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);   // Show button icon
			lv_label_set_text(ui_Label12, "0s");
			m_scanStartTime = 0;  // Reset for next attempt
		}
	}
}

/**
 * @brief Check for new TPMS sensors during scanning
 * @details Monitors State sensor map and detects when a new sensor appears.\n *          Front sensor: Selects first detected sensor\n *          Rear sensor: Selects first detected sensor different from front\n */
void PairController::checkForNewSensor() {
	// Only check during active scanning states
	if (m_state != PairingState::SCANNING_FRONT && m_state != PairingState::SCANNING_REAR) {
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

		if (m_state == PairingState::SCANNING_FRONT) {
			// Front sensor detected
			m_selectedFrontAddress = newestAddress;
			m_state = PairingState::WAITING_FRONT_CONFIRM;
			ESP_LOGI(TAG, "Front sensor found: %s", newestAddress.c_str());
			updateUI();
		} else if (m_state == PairingState::SCANNING_REAR) {
			// Rear sensor detected - verify it's different from front
			if (newestAddress != m_selectedFrontAddress) {
				m_selectedRearAddress = newestAddress;
				m_state = PairingState::WAITING_REAR_CONFIRM;
				ESP_LOGI(TAG, "Rear sensor found: %s", newestAddress.c_str());
				updateUI();
			} else {
				ESP_LOGW(TAG, "Ignoring sensor - same as front: %s", newestAddress.c_str());
			}
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
	if (m_state == PairingState::WAITING_FRONT_CONFIRM) {
		// Show front sensor address in green
		lv_label_set_text(ui_Label11, m_selectedFrontAddress.c_str());
		lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0x00FF00), LV_PART_MAIN);  // Green
		lv_obj_add_flag(ui_Spinner4, LV_OBJ_FLAG_HIDDEN);    // Hide spinner
		lv_obj_clear_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);   // Show button icon
		lv_label_set_text(ui_Label12, "---");
	} else if (m_state == PairingState::WAITING_REAR_CONFIRM) {
		// Show rear sensor address in green
		lv_label_set_text(ui_Label11, m_selectedRearAddress.c_str());
		lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0x00FF00), LV_PART_MAIN);  // Green
		lv_obj_add_flag(ui_Spinner4, LV_OBJ_FLAG_HIDDEN);    // Hide spinner
		lv_obj_clear_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);   // Show button icon
		lv_label_set_text(ui_Label12, "---");
	}
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

	if (m_state == PairingState::SCANNING_FRONT || m_state == PairingState::TIMEOUT_FRONT) {
		// Start or retry front sensor scan
		ESP_LOGI(TAG, "Starting/retrying front sensor scan");
		lv_label_set_text(ui_Label11, "SCANNING...");
		lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0xFFFFFF), LV_PART_MAIN);  // White
		lv_obj_clear_flag(ui_Spinner4, LV_OBJ_FLAG_HIDDEN);  // Show spinner
		lv_obj_add_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);     // Hide button icon
		m_state = PairingState::SCANNING_FRONT;
		m_scanStartTime = esp_timer_get_time() / 1000;  // Start timeout
	} else if (m_state == PairingState::SCANNING_REAR || m_state == PairingState::TIMEOUT_REAR) {
		// Start or retry rear sensor scan
		ESP_LOGI(TAG, "Starting/retrying rear sensor scan");
		lv_label_set_text(ui_Label11, "SCANNING...");
		lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0xFFFFFF), LV_PART_MAIN);  // White
		lv_obj_clear_flag(ui_Spinner4, LV_OBJ_FLAG_HIDDEN);  // Show spinner
		lv_obj_add_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);     // Hide button icon
		m_state = PairingState::SCANNING_REAR;
		m_scanStartTime = esp_timer_get_time() / 1000;  // Start timeout
	} else if (m_state == PairingState::WAITING_FRONT_CONFIRM) {
		// Front sensor confirmed - proceed to rear scan
		ESP_LOGI(TAG, "Front sensor confirmed, scanning rear");
		startRearScan();
	} else if (m_state == PairingState::WAITING_REAR_CONFIRM) {
		// Both sensors confirmed - save and reboot
		ESP_LOGI(TAG, "Rear sensor confirmed, saving and rebooting");
		savePairingAndReboot();
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
	// Validation check
	if (m_selectedFrontAddress.empty() || m_selectedRearAddress.empty()) {
		ESP_LOGE(TAG, "Error - missing sensor address");
		return;
	}

	ESP_LOGI(TAG, "Saving pairing - Front: %s, Rear: %s",
		   m_selectedFrontAddress.c_str(), m_selectedRearAddress.c_str());

	// Save addresses to NVS via ConfigManager
	Application &app = Application::instance();
	app.getConfig().setString("front_address", m_selectedFrontAddress);
	app.getConfig().setString("rear_address", m_selectedRearAddress);

	// Update global state
	State &state = State::getInstance();
	state.setFrontAddress(m_selectedFrontAddress);
	state.setRearAddress(m_selectedRearAddress);
	state.setIsPaired(true);

	// Update pairing controller state
	m_state = PairingState::COMPLETE;
	m_pairingComplete = true;

	// Show completion message
	lv_label_set_text(ui_Label11, "PAIRING COMPLETE");
	lv_obj_add_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);

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
