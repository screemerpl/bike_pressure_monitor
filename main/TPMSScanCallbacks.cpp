/**
 * @file TPMSScanCallbacks.cpp
 * @brief Implementation of BLE scan callbacks for TPMS sensor detection
 */

#include "TPMSScanCallbacks.h"
#include "State.h"           // Global state singleton
#include "TPMSUtil.h"        // TPMS data parser
#include "esp_log.h"         // ESP logging
#include <NimBLEDevice.h>    // BLE library
#include <string>            // std::string

/// Log tag for TPMSScanCallbacks module
static const char* TAG = "TPMSScan";

/**
 * @brief Process discovered BLE device and check if it's a TPMS sensor
 * @param advertisedDevice Pointer to discovered BLE device
 * @details Processing flow:
 *          1. Extract manufacturer data from advertisement
 *          2. Validate TPMS sensor format using TPMSUtil::isTPMSSensor() - raw pointer version
 *          3. Parse sensor data (pressure, temperature, battery, etc.)
 *          4. Add or update sensor in State map by MAC address
 *          5. Log sensor details with timestamp
 *          Note: Uses raw pointer validation to avoid std::string copy overhead in callback
 */
void TPMSScanCallbacks::onDiscovered(
    const NimBLEAdvertisedDevice *advertisedDevice) {

    // Extract manufacturer-specific data from BLE advertisement (raw pointer)
    std::string manufacturerData = advertisedDevice->getManufacturerData();
    const uint8_t* rawData = reinterpret_cast<const uint8_t*>(manufacturerData.c_str());

    // Check if this is a TPMS sensor using raw pointer version (avoids second string copy)
    // Validates: length=18, header 0x00 0x01, magic 0xEA 0xCA, sensor >= 0x80
    if (TPMSUtil::isTPMSSensor(rawData, manufacturerData.size())) {
        // Parse sensor data into TPMSUtil object
        TPMSUtil *sensor = TPMSUtil::parse(
            manufacturerData, advertisedDevice->getAddress().toString());

        // Add or update sensor in global state
        State& state = State::getInstance();
        std::string address = advertisedDevice->getAddress().toString();
        
        bool isNewSensor = false;
        bool dataChanged = false;
        
        // Check if sensor already exists in map
        if (!state.getData().contains(address)) {
            // New sensor - add to map
            state.getData()[address] = sensor;
            isNewSensor = true;
        } else {
            // Existing sensor - check if data changed before updating
            TPMSUtil *existingSensor = state.getData()[address];
            
            // Check if pressure, temperature, or battery changed
            dataChanged = (existingSensor->pressurePSI != sensor->pressurePSI) ||
                         (existingSensor->temperatureC != sensor->temperatureC) ||
                         (existingSensor->batteryLevel != sensor->batteryLevel) ||
                         (existingSensor->alert != sensor->alert);
            
            delete existingSensor;
            state.getData()[address] = sensor;
        }
        
        // Log only on new sensor or significant data change (not every advertisement)
        if (isNewSensor || dataChanged) {
            // Format timestamp as HH:MM:SS for log message
            uint64_t total_seconds = sensor->timestamp / 1000ULL;
            int hours = (total_seconds / 3600) % 24;
            int minutes = (total_seconds / 60) % 60;
            int seconds = total_seconds % 60;

            // Log sensor discovery/update with all details
            ESP_LOGI(TAG, "[%02d:%02d:%02d] TPMS %s at %s  id: 0x%02x%02x%02x, wheel "
                   "index: %d, pressure: %.1f PSI, temperature: %.1f C, battery: "
                   "%d%%, alert: %d",
                   hours, minutes, seconds,
                   isNewSensor ? "Sensor found" : "Data changed",
                   address.c_str(),
                   sensor->identifier[0], sensor->identifier[1],
                   sensor->identifier[2], sensor->sensorNumber, sensor->pressurePSI,
                   sensor->temperatureC, sensor->batteryLevel, sensor->alert);
        }

        // IMPORTANT: Do NOT delete sensor - it's now owned by state.getData() map
    }
}

/**
 * @brief Process scan result (not used in active scanning mode)
 * @param advertisedDevice Pointer to scanned device
 * @details onDiscovered() handles all sensor processing during active scanning
 */
void TPMSScanCallbacks::onResult(
	const NimBLEAdvertisedDevice *advertisedDevice) {
	// Not used - active scanning handled in onDiscovered
}

/**
 * @brief Restart scanning when scan window ends
 * @param results Scan results summary (unused)
 * @param reason Scan end reason code (unused)
 * @details Ensures continuous scanning by immediately restarting
 *          the BLE scan with same parameters (1000ms window)
 */
void TPMSScanCallbacks::onScanEnd(const NimBLEScanResults &results,
								  int reason) {
	// Automatically restart scanning for continuous operation
	NimBLEDevice::getScan()->start(SCAN_TIME_MS, false, true);
}