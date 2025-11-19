#include "PairController.h"
#include "Application.h"
#include "State.h"
#include "WiFiManager.h"
#include "WebServer.h"
#include "UI/ui.h"
#include "esp_timer.h"
#include "lvgl.h"
#include <NimBLEDevice.h>
#include <cstdio>

PairController &PairController::instance() {
	static PairController controller;
	return controller;
}

void PairController::init() {
	printf("PairController: Initializing pairing mode\n");
	
	m_state = PairingState::SCANNING_FRONT;
	m_selectedFrontAddress.clear();
	m_selectedRearAddress.clear();
	m_pairingComplete = false;
	m_scanStartTime = 0;  // Will be set when user presses button to start
	
	// Stop WiFi and WebServer during pairing for better BLE performance
	printf("PairController: Stopping WiFi for better BLE scanning\n");
	WebServer::instance().stop();
	WiFiManager::instance().stop();
	
	// Switch to active BLE scan for pairing
	NimBLEScan *pBLEScan = NimBLEDevice::getScan();
	pBLEScan->stop();
	pBLEScan->setActiveScan(true);
	pBLEScan->setInterval(100); // More aggressive for pairing
	pBLEScan->setWindow(99);
	pBLEScan->start(0, false, false);
	printf("PairController: Switched to active BLE scan\n");
	
	// Show initial UI - waiting for button press to start
	lv_label_set_text(ui_Label10, "-FRONT WHEEL-");
	lv_label_set_text(ui_Label11, "START PAIRING");
	lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0xFFFF00), LV_PART_MAIN);
	lv_obj_add_flag(ui_Spinner4, LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);
	lv_label_set_text(ui_Label12, "---");
}

void PairController::startFrontScan() {
	printf("PairController: Starting front wheel scan\n");
	
	m_state = PairingState::SCANNING_FRONT;
	m_scanStartTime = esp_timer_get_time() / 1000;
	m_lastSensorCount = 0;
	
	// Update UI
	lv_label_set_text(ui_Label10, "-FRONT WHEEL-");
	lv_label_set_text(ui_Label11, "START PAIRING");
	lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0xFFFF00), LV_PART_MAIN);
	lv_obj_add_flag(ui_Spinner4, LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);
	lv_label_set_text(ui_Label12, "60s");
}

void PairController::startRearScan() {
	printf("PairController: Starting rear wheel scan\n");
	
	m_state = PairingState::SCANNING_REAR;
	m_scanStartTime = esp_timer_get_time() / 1000;
	m_lastSensorCount = 0;
	
	// Update UI
	lv_label_set_text(ui_Label10, "-REAR WHEEL-");
	lv_label_set_text(ui_Label11, "START PAIRING");
	lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0xFFFF00), LV_PART_MAIN);
	lv_obj_add_flag(ui_Spinner4, LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);
	lv_label_set_text(ui_Label12, "60s");
}

void PairController::update(uint32_t currentTime) {
	if (m_state == PairingState::COMPLETE) {
		return;
	}

	// Check for new sensors
	checkForNewSensor();

	// Update timeout counter (only if scan has started)
	if ((m_state == PairingState::SCANNING_FRONT || m_state == PairingState::SCANNING_REAR) && m_scanStartTime > 0) {
		uint32_t elapsed = currentTime - m_scanStartTime;
		uint32_t remaining = (SCAN_TIMEOUT_MS - elapsed) / 1000;
		
		if (elapsed < SCAN_TIMEOUT_MS) {
			char timeoutText[16];
			snprintf(timeoutText, sizeof(timeoutText), "%lus", remaining);
			lv_label_set_text(ui_Label12, timeoutText);
		} else {
			// Timeout - show message and wait for button press
			printf("PairController: Scan timeout\n");
			if (m_state == PairingState::SCANNING_FRONT) {
				m_state = PairingState::TIMEOUT_FRONT;
			} else {
				m_state = PairingState::TIMEOUT_REAR;
			}
			lv_label_set_text(ui_Label11, "TIMEOUT");
			lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0xFFFF00), LV_PART_MAIN);
			lv_obj_add_flag(ui_Spinner4, LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);
			lv_label_set_text(ui_Label12, "0s");
			m_scanStartTime = 0;  // Reset for next attempt
		}
	}
}

void PairController::checkForNewSensor() {
	if (m_state != PairingState::SCANNING_FRONT && m_state != PairingState::SCANNING_REAR) {
		return;
	}

	State &state = State::getInstance();
	uint32_t currentCount = state.data.size();

	// Check if a new sensor appeared
	if (currentCount > m_lastSensorCount && currentCount > 0) {
		// Get the newest sensor address
		std::string newestAddress;
		for (const auto &pair : state.data) {
			newestAddress = pair.first;
			break; // Take first one for now
		}

		if (m_state == PairingState::SCANNING_FRONT) {
			m_selectedFrontAddress = newestAddress;
			m_state = PairingState::WAITING_FRONT_CONFIRM;
			printf("PairController: Front sensor found: %s\n", newestAddress.c_str());
			updateUI();
		} else if (m_state == PairingState::SCANNING_REAR) {
			// Make sure it's not the same as front sensor
			if (newestAddress != m_selectedFrontAddress) {
				m_selectedRearAddress = newestAddress;
				m_state = PairingState::WAITING_REAR_CONFIRM;
				printf("PairController: Rear sensor found: %s\n", newestAddress.c_str());
				updateUI();
			} else {
				printf("PairController: Ignoring sensor - same as front: %s\n", newestAddress.c_str());
			}
		}
	}

	m_lastSensorCount = currentCount;
}

void PairController::updateUI() {
	if (m_state == PairingState::WAITING_FRONT_CONFIRM) {
		// Show front sensor address
		lv_label_set_text(ui_Label11, m_selectedFrontAddress.c_str());
		lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0x00FF00), LV_PART_MAIN);
		lv_obj_add_flag(ui_Spinner4, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);
		lv_label_set_text(ui_Label12, "---");
	} else if (m_state == PairingState::WAITING_REAR_CONFIRM) {
		// Show rear sensor address
		lv_label_set_text(ui_Label11, m_selectedRearAddress.c_str());
		lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0x00FF00), LV_PART_MAIN);
		lv_obj_add_flag(ui_Spinner4, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);
		lv_label_set_text(ui_Label12, "---");
	}
}

void PairController::handleButtonPress() {
	printf("PairController: Button pressed in state %d\n", static_cast<int>(m_state));

	if (m_state == PairingState::SCANNING_FRONT || m_state == PairingState::TIMEOUT_FRONT) {
		// Start or retry front scan
		printf("PairController: Starting/retrying front sensor scan\n");
		lv_label_set_text(ui_Label11, "SCANNING...");
		lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
		lv_obj_clear_flag(ui_Spinner4, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);
		m_state = PairingState::SCANNING_FRONT;
		m_scanStartTime = esp_timer_get_time() / 1000;
	} else if (m_state == PairingState::SCANNING_REAR || m_state == PairingState::TIMEOUT_REAR) {
		// Start or retry rear scan
		printf("PairController: Starting/retrying rear sensor scan\n");
		lv_label_set_text(ui_Label11, "SCANNING...");
		lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
		lv_obj_clear_flag(ui_Spinner4, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);
		m_state = PairingState::SCANNING_REAR;
		m_scanStartTime = esp_timer_get_time() / 1000;
	} else if (m_state == PairingState::WAITING_FRONT_CONFIRM) {
		// Front confirmed, start rear scan
		printf("PairController: Front sensor confirmed, scanning rear\n");
		startRearScan();
	} else if (m_state == PairingState::WAITING_REAR_CONFIRM) {
		// Rear confirmed, save and reboot
		printf("PairController: Rear sensor confirmed, saving and rebooting\n");
		savePairingAndReboot();
	}
}

void PairController::savePairingAndReboot() {
	if (m_selectedFrontAddress.empty() || m_selectedRearAddress.empty()) {
		printf("PairController: Error - missing sensor address\n");
		return;
	}

	printf("PairController: Saving pairing - Front: %s, Rear: %s\n",
		   m_selectedFrontAddress.c_str(), m_selectedRearAddress.c_str());

	// Save to config
	Application &app = Application::instance();
	app.getConfig().setString("front_address", m_selectedFrontAddress);
	app.getConfig().setString("rear_address", m_selectedRearAddress);

	// Update state
	State &state = State::getInstance();
	state.frontAddress = m_selectedFrontAddress;
	state.rearAddress = m_selectedRearAddress;
	state.isPaired = true;

	m_state = PairingState::COMPLETE;
	m_pairingComplete = true;

	// Show completion message briefly
	lv_label_set_text(ui_Label11, "PAIRING COMPLETE");
	lv_obj_add_flag(ui_Label13, LV_OBJ_FLAG_HIDDEN);

	// Restore normal BLE scan parameters
	printf("PairController: Restoring normal BLE scan\n");
	NimBLEScan *pBLEScan = NimBLEDevice::getScan();
	pBLEScan->stop();
	pBLEScan->setActiveScan(true);
	pBLEScan->setInterval(100);
	pBLEScan->setWindow(50);
	pBLEScan->start(0, false, false);

	// Wait and reboot
	vTaskDelay(pdMS_TO_TICKS(1500));
	esp_restart();
}
