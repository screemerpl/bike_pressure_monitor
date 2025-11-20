/**
 * @file TPMSUtil_Type2.cpp
 * @brief Implementation of TPMS Type 2 sensor data parser
 * @details Parses 11-byte BLE manufacturer data packets from TC.TPMS Type 2 sensors
 * 
 * Example MAC: 37:39:02:00:d7:6a (wheel 2, unique ID: 00:d7:6a)
 * Example data: 01 1C 17 01 B3 (normal, 2.8V, 23°C, 30 PSI)
 * 
 * @author Artur Jakubowicz
 * @date 20 Nov 2025
 */

#include "TPMSUtil_Type2.h"
#include "esp_timer.h"   // High-resolution timer
#include <string.h>      // memcpy
#include <stdio.h>       // sscanf for MAC parsing

/**
 * @brief Construct TPMSUtil_Type2 from manufacturer data
 * @param address Sensor MAC address (e.g., "37:39:02:00:d7:6a")
 * @param data Pointer to 11-byte manufacturer data
 * @details Parses all sensor data fields and stores timestamp.
 *          Also copies values to legacy public members for compatibility.
 */
TPMSUtil_Type2::TPMSUtil_Type2(const std::string& address, const uint8_t* data)
	: m_address(address) {
	// Copy manufacturer data to internal array
	memcpy(m_manufacturerData, data, 11);
	
	// Parse all data fields from manufacturer data
	this->parseStatus();           // Extract alarm flag (bit 1)
	this->parseBattery();          // Extract battery voltage and percentage
	this->parseTemperature();      // Convert to Celsius
	this->parsePressure();         // Calculate PSI from 16-bit raw value
	this->parseWheelNumber(address);  // Extract wheel number from MAC
	this->parseSensorID(address);     // Extract unique ID from MAC
	
	// Record timestamp (milliseconds since boot)
	m_timestamp = esp_timer_get_time() / 1000;
}

/**
 * @brief Destructor
 */
TPMSUtil_Type2::~TPMSUtil_Type2() {
}

/**
 * @brief Validate if manufacturer data is from a TPMS Type 2 sensor
 * @param data Pointer to raw BLE manufacturer data
 * @param length Length of data (must be 11 bytes)
 * @return true if data matches TPMS Type 2 sensor specification
 * @details Validation checks:
 *          - Length must be exactly 11 bytes
 */
bool TPMSUtil_Type2::isTPMSSensor_Type2(const uint8_t* data, size_t length) {
	// Check length - Type 2 is always 11 bytes
	return (length == 11);
}

/**
 * @brief Factory method to create TPMSUtil_Type2 from manufacturer data
 * @param data Pointer to raw manufacturer data (11 bytes)
 * @param length Data length
 * @param address Sensor MAC address
 * @return Pointer to new TPMSUtil_Type2 object, or nullptr if data is invalid
 */
TPMSUtil_Type2 *TPMSUtil_Type2::parse(const uint8_t* data, size_t length, const std::string& address) {
	return TPMSUtil_Type2::isTPMSSensor_Type2(data, length)
				? new TPMSUtil_Type2(address, data)
				: nullptr;
}

/**
 * @brief Parse status byte and extract alarm flag
 * @details Alarm is determined by bit 1 of byte[0]:
 *          - If (byte[0] & 0x02) != 0 → alarm active
 *          - Otherwise → normal (both 0x00 and 0x01 are normal)
 */
void TPMSUtil_Type2::parseStatus() {
	uint8_t statusByte = m_manufacturerData[0];
	// Check bit 1 (0x02) - if set, alarm is active
	m_alarm = (statusByte & 0x02) != 0;
}

/**
 * @brief Parse battery data from byte[1]
 * @details Battery voltage:
 *          - Value = byte[1] * 0.1 (e.g., 28 = 2.8V, 31 = 3.1V)
 *          - Percentage = min((V - 2.0) * 100, 100)
 *          - Typical range: 2.0V-3.0V (0%-100%)
 */
void TPMSUtil_Type2::parseBattery() {
	uint8_t rawBattery = m_manufacturerData[1];
	m_batteryVoltage = rawBattery * 0.1f;
	
	// Calculate percentage (2.0V = 0%, 3.0V = 100%, capped at 100%)
	float percentage = (m_batteryVoltage - 2.0f) * 100.0f;
	m_batteryPercentage = (percentage > 100.0f) ? 100 : (uint8_t)percentage;
}

/**
 * @brief Parse temperature data from byte[2]
 * @details Temperature is stored as raw value in Celsius:
 *          - No scaling applied (unlike Type 1)
 *          - Value = byte[2] * 1.0 (direct Celsius)
 *          - Example: 23 = 23°C, 25 = 25°C
 */
void TPMSUtil_Type2::parseTemperature() {
	uint8_t rawTemp = m_manufacturerData[2];
	// Direct interpretation as Celsius (no scaling)
	m_temperatureC = (float)rawTemp;
}

/**
 * @brief Parse pressure data from bytes[3-4]
 * @details Pressure is stored as 16-bit little-endian value:
 *          - byte[3] = MSB (upper 8 bits)
 *          - byte[4] = LSB (lower 8 bits)
 *          - raw = byte[4] + byte[3]*256
 *          - PSI = 0.10223139 * raw - 14.61232950
 *          - Accuracy: ±0.14 PSI (verified from 4-point calibration)
 */
void TPMSUtil_Type2::parsePressure() {
	uint8_t msb = m_manufacturerData[3];
	uint8_t lsb = m_manufacturerData[4];
	
	// Combine to 16-bit LE value
	uint16_t rawValue = lsb + (msb << 8);
	
	// Apply calibration formula (derived from 4-point linear regression)
	m_pressurePSI = 0.10223139f * rawValue - 14.61232950f;
}

/**
 * @brief Parse wheel number from MAC address byte[2]
 * @details MAC format: 37:39:WW:XX:YY:ZZ
 *          WW (byte[2]) = wheel number (0x01=wheel1, 0x02=wheel2, etc.)
 */
void TPMSUtil_Type2::parseWheelNumber(const std::string& address) {
	// MAC format: "37:39:WW:XX:YY:ZZ"
	// Byte[2] contains wheel number
	unsigned int byte0, byte1, byte2, byte3, byte4, byte5;
	
	int matched = sscanf(address.c_str(), "%X:%X:%X:%X:%X:%X",
						&byte0, &byte1, &byte2, &byte3, &byte4, &byte5);
	
	if (matched == 6) {
		m_wheelNumber = byte2;
	} else {
		m_wheelNumber = 0;  // Default if parse fails
	}
}

/**
 * @brief Parse sensor ID from MAC address bytes[3-5]
 * @details MAC format: 37:39:WW:XX:YY:ZZ
 *          Bytes[3-5] (XX:YY:ZZ) = unique sensor identifier
 */
void TPMSUtil_Type2::parseSensorID(const std::string& address) {
	// MAC format: "37:39:WW:XX:YY:ZZ"
	// Bytes[3-5] contain unique sensor ID
	unsigned int byte0, byte1, byte2, byte3, byte4, byte5;
	
	int matched = sscanf(address.c_str(), "%X:%X:%X:%X:%X:%X",
						&byte0, &byte1, &byte2, &byte3, &byte4, &byte5);
	
	if (matched == 6) {
		m_sensorID[0] = byte3;
		m_sensorID[1] = byte4;
		m_sensorID[2] = byte5;
	} else {
		m_sensorID[0] = 0;
		m_sensorID[1] = 0;
		m_sensorID[2] = 0;
	}
}
