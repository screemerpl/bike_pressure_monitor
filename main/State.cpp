#include "State.h"
#include "esp_timer.h"
#include <cstdio>

State &State::getInstance() {
	static State instance;
	return instance;
}

void State::cleanupOldSensors() {
	uint64_t currentTime = esp_timer_get_time() / 1000; // Current time in ms
	uint64_t threshold =
		60000 * 7; // 7 minutes in ms (on idle it refreshes every 5 minutes)

	int removedCount = 0;
	auto it = data.begin();
	while (it != data.end()) {
		TPMSUtil *sensor = it->second;

		// Check if sensor is older than threshold
		if (currentTime - sensor->timestamp > threshold) {
			printf("Removing old sensor: %s\n", it->first.c_str());

			delete sensor;			 // Free memory
			it = data.erase(it);	 // Remove from map and get next iterator
			removedCount++;
		} else {
			++it; // Move to next element
		}
	}

	if (removedCount > 0) {
		uint64_t total_seconds = currentTime / 1000;
		int hours = (total_seconds / 3600) % 24;
		int minutes = (total_seconds / 60) % 60;
		int seconds = total_seconds % 60;

		printf("[%02d:%02d:%02d] Cleanup complete: removed %d sensors, %zu "
			   "sensors remaining in map\n",
			   hours, minutes, seconds, removedCount, data.size());
	}
}
