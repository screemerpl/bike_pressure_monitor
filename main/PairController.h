#pragma once

#include <cstdint>
#include <string>

class PairController {
public:
	static PairController &instance();

	// Initialize pairing screen
	void init();

	// Update UI based on scanning state
	void update(uint32_t currentTime);

	// Check if a new sensor was discovered
	void checkForNewSensor();

	// Handle button press - confirm current sensor and move to next
	void handleButtonPress();

	// Get current pairing state
	bool isPairingComplete() const { return m_pairingComplete; }

private:
	PairController() = default;
	~PairController() = default;

	PairController(const PairController &) = delete;
	PairController &operator=(const PairController &) = delete;

	enum class PairingState {
		SCANNING_FRONT,
		WAITING_FRONT_CONFIRM,
		SCANNING_REAR,
		WAITING_REAR_CONFIRM,
		COMPLETE
	};

	void updateUI();
	void startFrontScan();
	void startRearScan();
	void savePairingAndReboot();

	PairingState m_state = PairingState::SCANNING_FRONT;
	std::string m_selectedFrontAddress;
	std::string m_selectedRearAddress;
	uint32_t m_scanStartTime = 0;
	uint32_t m_lastSensorCount = 0;
	bool m_pairingComplete = false;

	static constexpr uint32_t SCAN_TIMEOUT_MS = 60000; // 60 seconds
};
