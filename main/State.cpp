/**
 * @file State.cpp
 * @brief Implementation of global application state singleton
 */

#include "State.h"
#include "esp_timer.h"  // High-resolution timer for timestamps
#include "esp_log.h"    // ESP logging

/// Log tag for State module
static const char* TAG = "State";

/**
 * @brief Get singleton instance (Meyer's singleton)
 * @return Reference to the State singleton
 */
State &State::getInstance() {
	static State instance;
	return instance;
}

/**
 * @brief Remove stale sensor data from the map
 * @details Iterates through all sensors and removes those that haven't
 *          been updated in the last 7 minutes. This prevents memory leaks
 *          from sensors that are no longer in range.
 * 
 * Cleanup threshold: 7 minutes (420 seconds)
 * Rationale: TPMS sensors typically transmit every 5 minutes when idle
 */
void State::cleanupOldSensors() {
	// Get current time in milliseconds
	uint64_t currentTime = esp_timer_get_time() / 1000;
	
	// Threshold: 7 minutes (sensors transmit every ~5 min when idle)
	uint64_t threshold = 60000 * 7;

	int removedCount = 0;
	auto it = m_data.begin();
	
	// Iterate through all sensors
	while (it != m_data.end()) {
		TPMSSensor *sensor = it->second;

		// Check if sensor data is stale (older than threshold)
		if (currentTime - sensor->getTimestamp() > threshold) {
			ESP_LOGD(TAG, "Removing old sensor: %s", it->first.c_str());

			delete sensor;			 // Free TPMSUtil object memory
			it = m_data.erase(it);	 // Remove from map and advance iterator
			removedCount++;
		} else {
			++it; // Sensor is still valid, check next
		}
	}

	// Log cleanup statistics if any sensors were removed
	if (removedCount > 0) {
		// Format current time as HH:MM:SS for log message
		uint64_t total_seconds = currentTime / 1000;
		int hours = (total_seconds / 3600) % 24;
		int minutes = (total_seconds / 60) % 60;
		int seconds = total_seconds % 60;

		ESP_LOGI(TAG, "[%02d:%02d:%02d] Cleanup complete: removed %d sensors, %zu "
			   "sensors remaining in map",
			   hours, minutes, seconds, removedCount, m_data.size());
	}
}
