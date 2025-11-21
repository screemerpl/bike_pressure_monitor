/**
 * @file TPMSScanCallbacks.h
 * @brief BLE scan callbacks for TPMS sensor discovery
 * @details Implements NimBLE scan callbacks to detect and parse TPMS sensor
 *          advertisements during continuous BLE scanning
 */

#pragma once

#include <NimBLEDevice.h>  // NimBLE library

/**
 * @class TPMSScanCallbacks
 * @brief Handles BLE scan events for TPMS sensor detection
 * @details Processes BLE advertisements, validates TPMS sensor format,
 *          and updates global State with sensor data. Automatically
 *          restarts scanning when scan window ends.
 */
class TPMSScanCallbacks : public NimBLEScanCallbacks {
public:
	/**
	 * @brief Called when a BLE device is discovered
	 * @param advertisedDevice Pointer to discovered device
	 * @details Checks if device is a TPMS sensor, parses data,
	 *          and adds/updates sensor in State map
	 */
    void onDiscovered(const NimBLEAdvertisedDevice *advertisedDevice) override;
    
	/**
	 * @brief Called for each scan result (not used)
	 * @param advertisedDevice Pointer to scanned device
	 * @details Active scanning handled in onDiscovered()
	 */
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override;
    
	/**
	 * @brief Called when scan window ends
	 * @param results Scan results summary
	 * @param reason Scan end reason code
	 * @details Automatically restarts scanning for continuous operation
	 */
    void onScanEnd(const NimBLEScanResults &results, int reason) override;

private:
    static constexpr uint32_t SCAN_TIME_MS = 1000;  ///< Scan window duration (1 second)
};