/**
 * @file TPMSScanCallbacks.cpp
 * @brief Implementation of BLE scan callbacks for TPMS sensor detection
 */

#include "TPMSScanCallbacks.h"
#include "State.h"           // Global state singleton
#include "TPMSUtil.h"        // TPMS Type 1 data parser
#include "TPMSUtil_Type2.h"  // TPMS Type 2 data parser
#include "esp_log.h"         // ESP logging
#include <NimBLEDevice.h>    // BLE library
#include <string>            // std::string

/// Log tag for TPMSScanCallbacks module
static const char* TAG = "TPMSScan";

/// Log tag for BLE discovery logger (reverse engineering mode)
static const char* TAG_BLE_LOG = "BLELogger";

/**
 * @brief Log all BLE device details for reverse engineering TPMS variants
 * @param advertisedDevice Pointer to discovered BLE device
 * @details Logs all available information for TPMS devices and potential Type 2 sensors:
 *          - MAC address and RSSI signal strength
 *          - Manufacturer-specific data (if present)
 *          - Service UUIDs (primary and complete list)
 *          - TX power level
 *          This enables capturing unknown TPMS protocol variants for analysis.
 */
static void logBLEDeviceDetails(const NimBLEAdvertisedDevice *advertisedDevice) {
	// Check if device name contains "TPMS" OR has service UUID 0xA828 (Type 2)
	bool isTPMS = false;
	bool isType2Candidate = false;
	
	if (advertisedDevice->haveName()) {
		std::string deviceName = advertisedDevice->getName();
		if (deviceName.find("TPMS") != std::string::npos) {
			isTPMS = true;
		}
	}
	
	if (advertisedDevice->haveServiceUUID()) {
		NimBLEUUID svcUUID = advertisedDevice->getServiceUUID();
		if (svcUUID.equals(NimBLEUUID("0xA828"))) {
			isType2Candidate = true;
		}
	}
	
	// Skip if neither TPMS nor Type 2 candidate
	if (!isTPMS && !isType2Candidate) {
		return;
	}
	
	std::string deviceName = advertisedDevice->haveName() ? advertisedDevice->getName() : "NoName";
	std::string deviceType = isTPMS ? "TPMS" : "Type2_Candidate";
	
	// Device matches criteria - log all details
	std::string address = advertisedDevice->getAddress().toString();
	int rssi = advertisedDevice->getRSSI();
	
	// Get service UUID for logging
	std::string svcUUIDStr = "None";
	if (advertisedDevice->haveServiceUUID()) {
		NimBLEUUID svcUUID = advertisedDevice->getServiceUUID();
		svcUUIDStr = svcUUID.toString();
	}
	
	// Log MAC and signal strength
	ESP_LOGI(TAG_BLE_LOG, "========== %s Device ==========", deviceType.c_str());
	ESP_LOGI(TAG_BLE_LOG, "MAC: %s | RSSI: %d dBm | Name: %s | Service: %s", 
		   address.c_str(), rssi, deviceName.c_str(), svcUUIDStr.c_str());
	
	// Log TX Power if available
	if (advertisedDevice->getTXPower() != 127) {  // 127 means "not available"
		ESP_LOGI(TAG_BLE_LOG, "TX Power: %d dBm", advertisedDevice->getTXPower());
	}
	
	// Log Manufacturer Data (if present)
	if (advertisedDevice->haveManufacturerData()) {
		std::string mfgData = advertisedDevice->getManufacturerData();
		ESP_LOGI(TAG_BLE_LOG, "Manufacturer Data (%zu bytes):", mfgData.size());
		
		// Print as hex dump: "00 01 02 03 04 05 06 07 08 09"
		std::string hexDump;
		for (size_t i = 0; i < mfgData.size(); ++i) {
			char hexByte[4];
			sprintf(hexByte, "%02X ", (unsigned char)mfgData[i]);
			hexDump += hexByte;
		}
		ESP_LOGI(TAG_BLE_LOG, "  %s", hexDump.c_str());
	} else {
		ESP_LOGI(TAG_BLE_LOG, "Manufacturer Data: None");
	}
	
	// Log Service UUIDs (Primary)
	if (advertisedDevice->haveServiceUUID()) {
		NimBLEUUID svcUUID = advertisedDevice->getServiceUUID();
		ESP_LOGI(TAG_BLE_LOG, "Primary Service UUID: %s", svcUUID.toString().c_str());
	}
	
	// Log Complete list of Service UUIDs with Data
	if (advertisedDevice->haveServiceData()) {
		ESP_LOGI(TAG_BLE_LOG, "Service Data: Present");
	}
	
	ESP_LOGI(TAG_BLE_LOG, "================================");
}

/**
 * @brief Process discovered BLE device and check if it's a TPMS sensor (Type 1 or Type 2)
 * @param advertisedDevice Pointer to discovered BLE device
 * @details Processing flow:
 *          1. Extract manufacturer data from advertisement
 *          2. Detect sensor type:
 *             - Type 1: 18 bytes, header 0x00 0x01, magic 0xEA 0xCA
 *             - Type 2: 11 bytes, service UUID 0xA828
 *          3. Parse appropriate sensor data format
 *          4. Add or update sensor in State map by MAC address
 *          5. Log sensor details with timestamp
 */
void TPMSScanCallbacks::onDiscovered(
    const NimBLEAdvertisedDevice *advertisedDevice) {

    // LOG ALL BLE DEVICES FOR REVERSE ENGINEERING
    // This helps identify new TPMS protocol variants
    logBLEDeviceDetails(advertisedDevice);

    // Extract manufacturer-specific data from BLE advertisement (raw pointer)
    std::string manufacturerData = advertisedDevice->getManufacturerData();
    const uint8_t* rawData = reinterpret_cast<const uint8_t*>(manufacturerData.c_str());
    std::string address = advertisedDevice->getAddress().toString();
    
    // State reference
    State& state = State::getInstance();
    bool isNewSensor = false;
    bool dataChanged = false;

    // ========================================================================
    // TYPE 1 DETECTION (18 bytes)
    // ========================================================================
    if (TPMSUtil::isTPMSSensor(rawData, manufacturerData.size())) {
        // Parse sensor data into TPMSUtil object
        TPMSUtil *sensor = TPMSUtil::parse(manufacturerData, address);

        // Check if sensor already exists in map
        if (!state.getData().contains(address)) {
            // New sensor - add to map
            state.getData()[address] = sensor;
            isNewSensor = true;
        } else {
            // Existing sensor - check if data changed before updating
            TPMSSensor *existingSensor = state.getData()[address];
            
            if (existingSensor) {
                // Check if pressure, temperature, battery, or alert changed
                dataChanged = (existingSensor->getPressurePSI() != sensor->getPressurePSI()) ||
                             (existingSensor->getTemperatureC() != sensor->getTemperatureC()) ||
                             (existingSensor->getBatteryLevel() != sensor->getBatteryLevel()) ||
                             (existingSensor->getAlert() != sensor->getAlert());
                
                // NOTE: Do NOT delete old sensor immediately - UI may still reference it
                // Just replace it in the map; cleanup task will handle memory later
            }
            state.getData()[address] = sensor;
        }
        
        // Log only on new sensor or significant data change (not every advertisement)
        if (isNewSensor || dataChanged) {
            // Format timestamp as HH:MM:SS for log message
            uint64_t total_seconds = sensor->getTimestamp() / 1000ULL;
            int hours = (total_seconds / 3600) % 24;
            int minutes = (total_seconds / 60) % 60;
            int seconds = total_seconds % 60;

            // Log sensor discovery/update with all details
            ESP_LOGI(TAG, "[%02d:%02d:%02d] TPMS Type 1 %s at %s  id: 0x%02x%02x%02x, wheel "
                   "index: %d, pressure: %.1f PSI, temperature: %.1f C, battery: "
                   "%d%%, alert: %d",
                   hours, minutes, seconds,
                   isNewSensor ? "Sensor found" : "Data changed",
                   address.c_str(),
                   sensor->getSensorID()[0], sensor->getSensorID()[1],
                   sensor->getSensorID()[2], sensor->getWheelNumber(), sensor->getPressurePSI(),
                   sensor->getTemperatureC(), sensor->getBatteryLevel(), sensor->getAlert());
        }

        // IMPORTANT: Do NOT delete sensor - it's now owned by state.getData() map
        return;
    }

    // ========================================================================
    // TYPE 2 DETECTION (11 bytes, service UUID 0xA828)
    // ========================================================================
    // Check if data is exactly 11 bytes first (Type 2 format)
    if (TPMSUtil_Type2::isTPMSSensor_Type2(rawData, manufacturerData.size())) {
        // Verify it matches Type 2 pattern:
        // - Service UUID 0xA828 (if present), OR
        // - MAC address starts with 37:39 (TC.TPMS manufacturer prefix)
        bool hasType2UUID = advertisedDevice->haveServiceUUID() && 
                           advertisedDevice->getServiceUUID().equals(NimBLEUUID("0xA828"));
        bool isTC_TPMS = (address.find("37:39") == 0);  // Starts with 37:39
        
        if (hasType2UUID || isTC_TPMS) {
            // Parse Type 2 sensor data
            TPMSUtil_Type2 *sensor = TPMSUtil_Type2::parse(rawData, manufacturerData.size(), address);
            
            if (sensor) {
                // Check if sensor already exists in map
                if (!state.getData().contains(address)) {
                    // New sensor - add to map
                    state.getData()[address] = sensor;
                    isNewSensor = true;
                } else {
                    // Existing sensor - check if data changed before updating
                    TPMSSensor *existingSensor = state.getData()[address];
                    
                    if (existingSensor) {
                        // Check if pressure, temperature, battery, or alarm changed
                        dataChanged = (existingSensor->getPressurePSI() != sensor->getPressurePSI()) ||
                                     (existingSensor->getTemperatureC() != sensor->getTemperatureC()) ||
                                     (existingSensor->getBatteryLevel() != sensor->getBatteryLevel()) ||
                                     (existingSensor->getAlert() != sensor->getAlert());
                        
                        // NOTE: Do NOT delete old sensor immediately - UI may still reference it
                        // Just replace it in the map; cleanup task will handle memory later
                    }
                    state.getData()[address] = sensor;
                }
                
                // Log only on new sensor or significant data change
                if (isNewSensor || dataChanged) {
                    // Format timestamp as HH:MM:SS for log message
                    uint64_t total_seconds = sensor->getTimestamp() / 1000ULL;
                    int hours = (total_seconds / 3600) % 24;
                    int minutes = (total_seconds / 60) % 60;
                    int seconds = total_seconds % 60;

                    // Log Type 2 sensor discovery/update
                    ESP_LOGI(TAG, "[%02d:%02d:%02d] TPMS Type 2 %s at %s  wheel: %d, id: 0x%02x%02x%02x, "
                           "pressure: %.2f PSI, temperature: %.0f C, battery: %.1fV (%d%%), alarm: %d",
                           hours, minutes, seconds,
                           isNewSensor ? "Sensor found" : "Data changed",
                           address.c_str(),
                           sensor->getWheelNumber(),
                           sensor->getSensorID()[0], sensor->getSensorID()[1], sensor->getSensorID()[2],
                           sensor->getPressurePSI(), sensor->getTemperatureC(), 
                           sensor->getBatteryVoltage(), sensor->getBatteryLevel(), sensor->getAlert());
                }
                
                // IMPORTANT: Do NOT delete sensor - it's now owned by state.getData() map
                return;
            }
        }
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