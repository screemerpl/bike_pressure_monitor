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
#include <vector>

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
		SCANNING_INDEX,         ///< Scanning for sensor at index (0..n-1)
		WAITING_INDEX_CONFIRM,  ///< A sensor for the current index has been found, waiting for user confirmation
		TIMEOUT_INDEX,          ///< Scan for the current index timed out, waiting for retry
		COMPLETE                ///< All sensors paired successfully
	};

	/** @brief Update pairing UI based on current state */
	void updateUI();
	
	/** @brief Async callback to update UI (thread-safe) */
	static void updateUICallback(void *arg);
	
	/** @brief Async callback to initialize pairing UI (thread-safe) */
	static void initUICallback(void *arg);
	
	/** @brief Async callback to start front scan UI (thread-safe) */
	static void startFrontScanUICallback(void *arg);
	
	/** @brief Async callback to start rear scan UI (thread-safe) */
	static void startRearScanUICallback(void *arg);
	
	/** @brief Async callback to update timeout text (thread-safe) */
	static void updateTimeoutCallback(void *arg);
	
	/** @brief Async callback to show timeout UI (thread-safe) */
	static void timeoutUICallback(void *arg);
	
	/** @brief Async callback to show scanning UI (thread-safe) */
	static void startScanningUICallback(void *arg);
	
	/** @brief Async callback to show pairing complete UI (thread-safe) */
	static void pairingCompleteUICallback(void *arg);
	
	/** @brief Start front wheel scan with timeout */
	void startFrontScan();
	void startRearScan();
	/** @brief Start scan for arbitrary index (0..n-1) */
	void startIndexScan(int index);
	
	/** @brief Save paired addresses to NVS and reboot */
	void savePairingAndReboot();

	PairingState m_state = PairingState::SCANNING_INDEX;  ///< Current pairing state
	std::vector<std::string> m_selectedAddresses;         ///< Selected sensor MAC addresses during pairing
	int m_expectedCount = 2;                              ///< Number of sensors to pair (2 or 4)
	int m_currentIndex = 0;                               ///< Current index being paired
	uint32_t m_scanStartTime = 0;                          ///< Scan start timestamp (ms)
	uint32_t m_lastSensorCount = 0;                        ///< Previous sensor count for detection
	bool m_pairingComplete = false;                        ///< Pairing completion flag
	std::string m_pendingTimeoutText;                      ///< Pending timeout text for async update

	static constexpr uint32_t SCAN_TIMEOUT_MS = 60000;     ///< Scan timeout: 60 seconds
};
