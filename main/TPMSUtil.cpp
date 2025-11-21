/**
 * @file TPMSUtil.cpp
 * @brief Implementation of TPMS sensor data parser
 * @details Parses 18-byte BLE manufacturer data packets from TPMS sensors
 * 
 * Example data: 0x00 01 81 EA CA 20 04 10 23 06 00 00 1F 0B 00 00 09 00
 * 
 * @author Artur Jakubowicz
 * @date 16 Nov 2025
 */

#include "TPMSUtil.h"
#include "stdlib.h"      // Standard library
#include "string.h"      // String manipulation (memcpy)
#include <esp_timer.h>   // High-resolution timer

/**
 * @brief Construct TPMSUtil from manufacturer data
 * @param address Sensor MAC address (e.g., "AA:BB:CC:DD:EE:FF")
 * @param manufacturerData Raw 18-byte BLE manufacturer data
 * @details Parses all sensor data fields and stores timestamp.
 *          Also copies values to legacy public members for compatibility.
 */
TPMSUtil::TPMSUtil(const std::string& address, const std::string& manufacturerData) 
	: m_address(address) {
	// Copy manufacturer data to internal array
	for (size_t i = 0; i < 18 && i < manufacturerData.size(); ++i) {
		m_manufacturerData[i] = static_cast<unsigned char>(manufacturerData[i]);
	}
	
	// Parse all data fields from manufacturer data
	this->parseID();           // Extract sensor ID and number
	this->parsePressure();     // Calculate PSI and BAR from kPa
	this->parseTemperature();  // Convert to Celsius
	this->parseOther();        // Extract battery and alert
	
	// Record timestamp (milliseconds since boot)
	this->timestamp = esp_timer_get_time() / 1000;
	this->m_timestamp = this->timestamp;
	
	// Copy to legacy public members for backward compatibility
	memcpy(this->identifier, m_identifier.data(), 3);
	this->sensorNumber = m_sensorNumber;
	this->pressurePSI = m_pressurePSI;
	this->pressureBar = m_pressureBar;
	this->temperatureC = m_temperatureC;
	this->batteryLevel = m_batteryLevel;
	this->alert = m_alert;
}

/**
 * @brief Destructor
 */
TPMSUtil::~TPMSUtil() {
}

/**
 * @brief Validate if manufacturer data is from a TPMS sensor
 * @param manufacturerData Raw BLE manufacturer data
 * @return true if data format matches TPMS sensor specification
 * @details Validation checks:
 *          1. Length must be exactly 18 bytes
 *          2. Header bytes [0-1] must be 0x00 0x01
 *          3. Magic bytes [3-4] must be 0xEA 0xCA
 *          4. Sensor number byte [2] must be >= 0x80
 */
bool TPMSUtil::isTPMSSensor(const std::string& manufacturerData) {
	const unsigned char *dataP = reinterpret_cast<const unsigned char *>(manufacturerData.c_str());
	return isTPMSSensor(dataP, manufacturerData.size());
}

/**
 * @brief Validate if manufacturer data is from a TPMS sensor (raw pointer version)
 * @param data Pointer to raw BLE manufacturer data
 * @param length Length of data (must be 18 bytes)
 * @return true if data format matches TPMS sensor specification
 * @details Optimized version for direct use from callback handlers - avoids string copy overhead.
 *          Validation checks:
 *          1. Length must be exactly 18 bytes
 *          2. Header bytes [0-1] must be 0x00 0x01
 *          3. Magic bytes [3-4] must be 0xEA 0xCA
 *          4. Sensor number byte [2] must be >= 0x80
 */
bool TPMSUtil::isTPMSSensor(const uint8_t* data, size_t length) {
	// Check length
	if (length != 18)
		return false;
	
	// Check header bytes (0x00 0x01)
	if (data[0] != 0x00 || data[1] != 0x01)
		return false;
	
	// Check magic bytes (0xEA 0xCA)
	if (data[3] != 0xea || data[4] != 0xca)
		return false;
	
	// Check sensor number (must be >= 0x80)
	if (data[2] < 0x80)
		return false;

	return true;
}

/**
 * @brief Factory method to create TPMSUtil from manufacturer data
 * @param manufacturerData Raw BLE manufacturer data (18 bytes)
 * @param address Sensor MAC address
 * @return Pointer to new TPMSUtil object, or nullptr if data is invalid
 * @details Validates data format before creating object.
 *          Caller must delete returned pointer when done.
 */
TPMSUtil *TPMSUtil::parse(const std::string& manufacturerData, const std::string& address) {
	return TPMSUtil::isTPMSSensor(manufacturerData)
				? new TPMSUtil(address, manufacturerData)
				: nullptr;
}

/**
 * @brief Extract 32-bit little-endian value from manufacturer data
 * @param index Starting byte index (0-14, must allow 4 bytes)
 * @return 32-bit integer value
 * @details Combines 4 consecutive bytes in little-endian order:
 *          result = byte[i] | (byte[i+1] << 8) | (byte[i+2] << 16) | (byte[i+3] << 24)
 */
long TPMSUtil::getLongValue(int index) const {
	long result = m_manufacturerData[index] |
					(long)(m_manufacturerData[index + 1]) << 8 |
					(long)(m_manufacturerData[index + 2]) << 16 |
					(long)m_manufacturerData[index + 3] << 24;
	return result;
}

/**
 * @brief Parse sensor ID and number from manufacturer data
 * @details Extracts:
 *          - Sensor ID: 3 bytes at [5-7]
 *          - Sensor number: byte [2] minus 0x80 (typically 1-4)
 */
void TPMSUtil::parseID() {
	// Copy 3-byte sensor identifier
	memcpy(m_identifier.data(), m_manufacturerData.data() + 5, 3);
	
	// Extract sensor number (0x80 = sensor 0, 0x81 = sensor 1, etc.)
	m_sensorNumber = m_manufacturerData[2] - 0x80;
}

/**
 * @brief Parse pressure data and convert to PSI and BAR
 * @details Pressure is stored as kPa * 1000 in bytes [8-11].
 *          Conversion factors:
 *          - PSI = kPa * 0.145037738
 *          - BAR = kPa / 100
 */
void TPMSUtil::parsePressure() {
	// Extract pressure value (kPa * 1000)
	long result = this->getLongValue(8);
	float kPA = result / 1000.0f;
	
	// Convert to PSI and BAR
	m_pressurePSI = kPA * 0.14503773773020923f;  // kPa to PSI
	m_pressureBar = kPA / 100.0f;                // kPa to BAR
}

/**
 * @brief Parse temperature data and convert to Celsius
 * @details Temperature is stored as °C * 100 in bytes [12-15]
 */
void TPMSUtil::parseTemperature() {
	// Extract temperature value (°C * 100)
	long result = this->getLongValue(12);
	float celsius = result / 100.0f;
	
	m_temperatureC = celsius;
}

/**
 * @brief Parse battery level and alert flag
 * @details Extracts:
 *          - Battery level: byte [16] (0-255)
 *          - Alert flag: byte [17] (1 = alert active, 0 = normal)
 */
void TPMSUtil::parseOther() {
	m_batteryLevel = m_manufacturerData[16];
	m_alert = (m_manufacturerData[17] == 1);
}
