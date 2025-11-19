#ifndef STATE_H
#define STATE_H

#include "TPMSUtil.h"
#include <string>
#include <unordered_map>

class State {
private:
	State() = default;
	State(const State &) = delete;
	State &operator=(const State &) = delete;
	State(State &&) = delete;
	State &operator=(State &&) = delete;

public:
	std::unordered_map<std::string, TPMSUtil *> data;
	std::string frontAddress = "";
	std::string rearAddress = "";
	bool isInAlertState = false;
	bool isPaired = false;
	float frontIdealPSI = 0.0f;
	float rearIdealPSI = 0.0f;
	std::string pressureUnit = "PSI"; // "PSI" or "BAR"

	static State &getInstance();

	// Clean up sensors older than threshold
	void cleanupOldSensors();
};

#endif // STATE_H