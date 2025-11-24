/**
 * @file TPMSUtil.h
 * @brief TPMS (Tire Pressure Monitoring System) sensor data parser
 * @details Parses BLE advertisement data from TPMS sensors and extracts:
 *          - Tire pressure (PSI and BAR)
 *          - Temperature (Celsius)
 *          - Battery level
 *          - Alert status
 *          - Sensor identification
 * 
 * Manufacturer data format (18 bytes):
 * [0-1]:   Header (0x00 0x01)
 * [2]:     Sensor number + 0x80
 * [3-4]:   Magic bytes (0xEA 0xCA)
 * [5-7]:   Sensor ID (3 bytes)
 * [8-11]:  Pressure (kPa * 1000)
 * [12-15]: Temperature (°C * 100)
 * [16]:    Battery level
 * [17]:    Alert flag
 * 
 * @author Artur Jakubowicz
 * @date 16 Nov 2025
 */

#ifndef MAIN_TPMSUTIL_H_
#define MAIN_TPMSUTIL_H_

#include "TPMSSensor.h"  // Base class
#include <string>      // std::string
#include <cstdint>     // uint64_t
#include <array>       // std::array

/**
 * @class TPMSUtil
 * @brief TPMS Type 1 sensor data container and parser
 * @details Validates and parses BLE manufacturer data from TPMS sensors.
 *          Provides both PSI and BAR pressure readings, temperature,
 *          battery status, and alert flags.
 *          Inherits from TPMSSensor for polymorphic handling.
 */
class TPMSUtil : public TPMSSensor {
public:
	virtual ~TPMSUtil();
	
	// ========================================================================
	// Base class interface implementation
	// ========================================================================
	std::string getSensorType() const override { return "Type1"; }
	float getPressurePSI() const override { return m_pressurePSI; }
	float getPressureBar() const override { return m_pressureBar; }
	float getTemperatureC() const override { return m_temperatureC; }
	const std::string& getAddress() const override { return m_address; }
	uint64_t getTimestamp() const override { return m_timestamp; }
	bool getAlert() const override { return m_alert; }
	uint8_t getBatteryLevel() const override { return static_cast<uint8_t>(m_batteryLevel); }  // Returns 0-255 for Type 1
	uint8_t getWheelNumber() const override { return static_cast<uint8_t>(m_sensorNumber); }
	const uint8_t* getSensorID() const override { return reinterpret_cast<const uint8_t*>(m_identifier.data()); }
	
	/**
	 * @brief Check if manufacturer data is from a TPMS sensor
	 * @param manufacturerData Raw BLE manufacturer data (18 bytes)
	 * @return true if data matches TPMS sensor format
	 * @details Validates:
	 *          - Data length (must be 18 bytes)
	 *          - Header bytes (0x00 0x01)
	 *          - Magic bytes at [3-4] (0xEA 0xCA)
	 *          - Sensor number >= 0x80
	 */
	static bool isTPMSSensor(const std::string& manufacturerData);
	
	/**
	 * @brief Check if manufacturer data is from a TPMS sensor (raw pointer version)
	 * @param data Pointer to raw BLE manufacturer data
	 * @param length Length of data (must be 18 bytes)
	 * @return true if data matches TPMS sensor format
	 * @details Optimized version for direct use from callback handlers - avoids string copy overhead
	 */
	static bool isTPMSSensor(const uint8_t* data, size_t length);
	
	/**
	 * @brief Parse manufacturer data and create TPMSUtil object
	 * @param manufacturerData Raw BLE manufacturer data (18 bytes)
	 * @param address Sensor MAC address
	 * @return Pointer to new TPMSUtil object, or nullptr if invalid
	 * @details Allocates new TPMSUtil on heap if data is valid.
	 *          Caller is responsible for deleting the returned pointer.
	 */
	static TPMSUtil *parse(const std::string& manufacturerData, const std::string& address);

	// Getters (legacy, for backward compatibility)
	
	/** @brief Get sensor identifier (3-character array) */
	const std::array<char, 3>& getIdentifier() const { return m_identifier; }
	
	/** @brief Get sensor number (1-4 for typical 4-wheel setup) */
	char getSensorNumber_Legacy() const { return m_sensorNumber; }

private:
	/**
	 * @brief Private constructor - use parse() factory method
	 * @param address Sensor MAC address
	 * @param manufacturerData Raw BLE manufacturer data
	 */
	TPMSUtil(const std::string& address, const std::string& manufacturerData);

	/**
	 * @brief Extract 4-byte little-endian value from manufacturer data
	 * @param index Starting byte index (0-14)
	 * @return 32-bit value
	 */
	long getLongValue(int index) const;
	
	/** @brief Parse pressure data from bytes [8-11] */
	void parsePressure();
	
	/** @brief Parse temperature data from bytes [12-15] */
	void parseTemperature();
	
	/** @brief Parse sensor ID and number from bytes [2,5-7] */
	void parseID();
	
	/** @brief Parse battery level and alert flag from bytes [16-17] */
	void parseOther();

	std::array<unsigned char, 18> m_manufacturerData;  ///< Raw manufacturer data
	std::string m_address;                             ///< Sensor MAC address
	
	// Private member variables
	std::array<char, 3> m_identifier;  ///< Sensor identifier (3 chars)
	char m_sensorNumber;               ///< Sensor number (1-4)
	float m_pressurePSI;               ///< Pressure in PSI
	float m_pressureBar;               ///< Pressure in BAR
	float m_temperatureC;              ///< Temperature in Celsius
	char m_batteryLevel;               ///< Battery level (0-255)
	bool m_alert;                      ///< Alert flag
	uint64_t m_timestamp;              ///< Last update timestamp (ms)
};

#endif /* MAIN_TPMSUTIL_H_ */
