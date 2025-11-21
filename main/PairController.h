/**
 * @file PairController.h
 * @brief Sensor pairing workflow state machine
 * @details Guides user through pairing process for front and rear TPMS sensors.
 *          Manages timeout handling, sensor selection, and configuration saving.
 * 
 * Pairing flow:
 * 1. Start front wheel scan (60s timeout)
 * 2. Detect first TPMS sensor -> Wait for button confirm
 * 3. Start rear wheel scan (60s timeout)
 * 4. Detect second TPMS sensor (different from front) -> Wait for confirm
 * 5. Save addresses to NVS and reboot
 */

#pragma once

#include <cstdint>  // uint32_t
#include <string>   // std::string

/**
 * @class PairController
 * @brief State machine for TPMS sensor pairing workflow
 * @details Manages UI updates, BLE scanning, timeout handling, and sensor selection
 *          during the pairing process. Stops WiFi during pairing for better BLE performance.
 */
class PairController {
public:
	/**
	 * @brief Get singleton instance
	 * @return Reference to the PairController singleton
	 */
	static PairController &instance();

	/**
	 * @brief Initialize pairing screen and start pairing mode
	 * @details Stops WiFi/WebServer, switches to active BLE scan,
	 *          and displays initial UI (waiting for button press)
	 */
	void init();

	/**
	 * @brief Update pairing state machine and UI
	 * @param currentTime Current timestamp in milliseconds
	 * @details Called periodically from main control loop.
	 *          Updates timeout counter and checks for new sensors.
	 */
	void update(uint32_t currentTime);

	/**
	 * @brief Check if a new TPMS sensor was discovered
	 * @details Monitors State sensor map and updates pairing state
	 *          when a new sensor appears during scanning.
	 */
	void checkForNewSensor();

	/**
	 * @brief Handle button press during pairing
	 * @details Actions depend on current state:
	 *          - SCANNING: Start scan with timeout
	 *          - WAITING_CONFIRM: Confirm sensor and move to next step
	 *          - TIMEOUT: Retry scan
	 *          - COMPLETE: Save addresses and reboot
	 */
	void handleButtonPress();

	/**
	 * @brief Check if pairing is complete
	 * @return true if both sensors are paired
	 */
	bool isPairingComplete() const { return m_pairingComplete; }

private:
	PairController() = default;                            ///< Private constructor for singleton
	~PairController() = default;                           ///< Private destructor

	PairController(const PairController &) = delete;       ///< No copy constructor
	PairController &operator=(const PairController &) = delete;  ///< No copy assignment

	/**
	 * @enum PairingState
	 * @brief Pairing workflow states
	 */
	enum class PairingState {
		SCANNING_FRONT,         ///< Scanning for front wheel sensor
		WAITING_FRONT_CONFIRM,  ///< Front sensor found, waiting for button confirm
		SCANNING_REAR,          ///< Scanning for rear wheel sensor
		WAITING_REAR_CONFIRM,   ///< Rear sensor found, waiting for button confirm
		TIMEOUT_FRONT,          ///< Front scan timeout, waiting for retry
		TIMEOUT_REAR,           ///< Rear scan timeout, waiting for retry
		COMPLETE                ///< Both sensors paired successfully
	};

	/** @brief Update pairing UI based on current state */
	void updateUI();
	
	/** @brief Start front wheel scan with timeout */
	void startFrontScan();
	
	/** @brief Start rear wheel scan with timeout */
	void startRearScan();
	
	/** @brief Save paired addresses to NVS and reboot */
	void savePairingAndReboot();

	PairingState m_state = PairingState::SCANNING_FRONT;  ///< Current pairing state
	std::string m_selectedFrontAddress;                    ///< Front sensor MAC address
	std::string m_selectedRearAddress;                     ///< Rear sensor MAC address
	uint32_t m_scanStartTime = 0;                          ///< Scan start timestamp (ms)
	uint32_t m_lastSensorCount = 0;                        ///< Previous sensor count for detection
	bool m_pairingComplete = false;                        ///< Pairing completion flag

	static constexpr uint32_t SCAN_TIMEOUT_MS = 60000;     ///< Scan timeout: 60 seconds
};
