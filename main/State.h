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

#include "TPMSSensor.h"     // Base class for polymorphic sensor handling
#include <string>           // std::string
#include <unordered_map>    // std::unordered_map

#define SENSOR_BIKE_FRONT 0
#define SENSOR_BIKE_FRONT_TEXT  "Front Wheel"
#define SENSOR_BIKE_REAR 1
#define SENSOR_BIKE_REAR_TEXT  "Rear Wheel"
#define SENSOR_CAR_FRONT_LEFT 0
#define SENSOR_CAR_FRONT_LEFT_TEXT  "Front Left"
#define SENSOR_CAR_REAR_LEFT 1
#define SENSOR_CAR_REAR_LEFT_TEXT  "Rear Left"
#define SENSOR_CAR_FRONT_RIGHT 2
#define SENSOR_CAR_FRONT_RIGHT_TEXT  "Front Right"
#define SENSOR_CAR_REAR_RIGHT 3
#define SENSOR_CAR_REAR_RIGHT_TEXT  "Rear Right"

#define MODE_BIKE 0
#define MODE_CAR 1
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
	
	/**
	 * @brief Get sensor data map (const version)
	 * @details Returns map of MAC address -> sensor data (Type 1 or Type 2)
	 */
	const std::unordered_map<std::string, TPMSSensor*>& getData() const { return m_data; }
	/**
	 * @brief Get sensor data map (mutable version)
	 * @details Returns map of MAC address -> sensor data (Type 1 or Type 2)
	 */
	std::unordered_map<std::string, TPMSSensor*>& getData() { return m_data; }
	
	/** @brief Get front sensor MAC address */
	const std::string& getAddress(int index) const { return m_sensorAddresses[index]; }
	/** @brief Set front sensor MAC address */
	void setAddress(int index, const std::string& address) { m_sensorAddresses[index] = address; }
	
	
	/** @brief Check if system is in alert state (low/high pressure warning) */
	bool getIsInAlertState() const { return m_isInAlertState; }
	/** @brief Set alert state flag */
	void setIsInAlertState(bool state) { m_isInAlertState = state; }
	
	/** @brief Check if sensors are paired (both front and rear configured) */
	bool getIsPaired() const { return m_isPaired; }
	/** @brief Set pairing status */
	void setIsPaired(bool paired) { m_isPaired = paired; }
	
	/** @brief Get ideal front tire pressure in PSI */
	float getIdealPSI(int index) const { return m_idealPressures[index]; }
	/** @brief Set ideal front tire pressure in PSI */
	void setIdealPSI(int index, float psi) { m_idealPressures[index] = psi; }
	
	/** @brief Get pressure unit preference ("PSI" or "BAR") */
	const std::string& getPressureUnit() const { return m_pressureUnit; }
	/** @brief Set pressure unit preference */
	void setPressureUnit(const std::string& unit) { m_pressureUnit = unit; }

	int getMode() const { return m_mode; }
	void setMode(int mode) { m_mode = mode; }
	/** @brief Get current UI theme index */
	int getUITheme() const { return m_uiTheme; }
	/** @brief Set current UI theme index */
	void setUITheme(int theme) { m_uiTheme = theme; }

private:
	State() = default;                           ///< Private constructor for singleton
	State(const State &) = delete;               ///< No copy constructor
	State &operator=(const State &) = delete;    ///< No copy assignment
	State(State &&) = delete;                    ///< No move constructor
	State &operator=(State &&) = delete;         ///< No move assignment
	
	// Private member variables
	std::unordered_map<std::string, TPMSSensor*> m_data;  ///< Map of MAC address -> sensor data (Type 1 or Type 2)
	//std::string m_frontAddress = "";                     ///< Front sensor MAC address (deprecated)
	//std::string m_rearAddress = "";                      ///< Rear sensor MAC address (deprecated)
	bool m_isInAlertState = false;                       ///< Alert state flag (pressure warning)
	bool m_isPaired = false;                             ///< Pairing status (both sensors configured)
	//float m_frontIdealPSI = 0.0f;                        ///< Target front tire pressure (PSI) (deprecated)
	//float m_rearIdealPSI = 0.0f;                         ///< Target rear tire pressure (PSI) (deprecated)
	std::string m_pressureUnit = "PSI";                  ///< Display unit: "PSI" or "BAR"

	std::string m_sensorAddresses[4];                    ///< Array of paired sensor addresses max 4
	float m_idealPressures[4];                          ///< Array of ideal pressures for up to 4 tires
	int m_mode = MODE_BIKE;                             ///< Operating mode: bike or car
	int m_uiTheme = 0;                                  ///< UI theme index (UI_THEME_DEFAULT)

};

#endif // STATE_H