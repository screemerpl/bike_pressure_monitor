/**
 * @file State.h
 * @brief Global application state singleton
 * @details Manages shared state across all application components including:
 *          - TPMS sensor data collection
 *          - Sensor pairing addresses
 *          - Ideal pressure values
 *          - Alert state
 *          - Pressure unit preference
 */

#ifndef STATE_H
#define STATE_H

#include "TPMSUtil.h"         // TPMS sensor data structures
#include <string>              // std::string
#include <unordered_map>       // std::unordered_map

/**
 * @class State
 * @brief Global singleton managing application state
 * @details Central repository for:
 *          - Active sensor data (map of MAC address -> TPMSUtil)
 *          - Paired sensor addresses (front/rear)
 *          - Target pressure values for each tire
 *          - Alert state for UI feedback
 *          - Pressure unit preference (PSI/BAR)
 * 
 * Thread-safety: Not thread-safe. Access from single task or use LVGL async calls.
 */
class State {
public:
	/**
	 * @brief Get singleton instance
	 * @return Reference to the State singleton
	 */
	static State &getInstance();

	/**
	 * @brief Remove sensors that haven't been seen in 7 minutes
	 * @details Frees memory and removes stale entries from sensor map.
	 *          Logs cleanup statistics with timestamp.
	 */
	void cleanupOldSensors();
	
	// Getters and Setters
	
	/** @brief Get sensor data map (const version) */
	const std::unordered_map<std::string, TPMSUtil *>& getData() const { return m_data; }
	/** @brief Get sensor data map (mutable version) */
	std::unordered_map<std::string, TPMSUtil *>& getData() { return m_data; }
	
	/** @brief Get front sensor MAC address */
	const std::string& getFrontAddress() const { return m_frontAddress; }
	/** @brief Set front sensor MAC address */
	void setFrontAddress(const std::string& address) { m_frontAddress = address; }
	
	/** @brief Get rear sensor MAC address */
	const std::string& getRearAddress() const { return m_rearAddress; }
	/** @brief Set rear sensor MAC address */
	void setRearAddress(const std::string& address) { m_rearAddress = address; }
	
	/** @brief Check if system is in alert state (low/high pressure warning) */
	bool getIsInAlertState() const { return m_isInAlertState; }
	/** @brief Set alert state flag */
	void setIsInAlertState(bool state) { m_isInAlertState = state; }
	
	/** @brief Check if sensors are paired (both front and rear configured) */
	bool getIsPaired() const { return m_isPaired; }
	/** @brief Set pairing status */
	void setIsPaired(bool paired) { m_isPaired = paired; }
	
	/** @brief Get ideal front tire pressure in PSI */
	float getFrontIdealPSI() const { return m_frontIdealPSI; }
	/** @brief Set ideal front tire pressure in PSI */
	void setFrontIdealPSI(float psi) { m_frontIdealPSI = psi; }
	
	/** @brief Get ideal rear tire pressure in PSI */
	float getRearIdealPSI() const { return m_rearIdealPSI; }
	/** @brief Set ideal rear tire pressure in PSI */
	void setRearIdealPSI(float psi) { m_rearIdealPSI = psi; }
	
	/** @brief Get pressure unit preference ("PSI" or "BAR") */
	const std::string& getPressureUnit() const { return m_pressureUnit; }
	/** @brief Set pressure unit preference */
	void setPressureUnit(const std::string& unit) { m_pressureUnit = unit; }

private:
	State() = default;                           ///< Private constructor for singleton
	State(const State &) = delete;               ///< No copy constructor
	State &operator=(const State &) = delete;    ///< No copy assignment
	State(State &&) = delete;                    ///< No move constructor
	State &operator=(State &&) = delete;         ///< No move assignment
	
	// Private member variables
	std::unordered_map<std::string, TPMSUtil *> m_data;  ///< Map of MAC address -> sensor data
	std::string m_frontAddress = "";                     ///< Front sensor MAC address
	std::string m_rearAddress = "";                      ///< Rear sensor MAC address
	bool m_isInAlertState = false;                       ///< Alert state flag (pressure warning)
	bool m_isPaired = false;                             ///< Pairing status (both sensors configured)
	float m_frontIdealPSI = 0.0f;                        ///< Target front tire pressure (PSI)
	float m_rearIdealPSI = 0.0f;                         ///< Target rear tire pressure (PSI)
	std::string m_pressureUnit = "PSI";                  ///< Display unit: "PSI" or "BAR"
};

#endif // STATE_H