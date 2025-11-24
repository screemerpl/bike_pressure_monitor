/**
 * @file TPMSUtil_Type2.h
 * @brief TPMS Type 2 (TC.TPMS) sensor data parser
 * @details Parses BLE advertisement data from TPMS Type 2 sensors (service UUID 0xA828)
 *          and extracts:
 *          - Tire pressure (PSI)
 *          - Temperature (Celsius)
 *          - Battery level (voltage and percentage)
 *          - Alarm status (based on bit 1 of status byte)
 *          - Sensor identification (MAC-based: wheel number + unique ID)
 * 
 * Manufacturer data format (11 bytes):
 * [0]:     Status byte (bit 1 = alarm flag, 0x00/0x01 = normal)
 * [1]:     Battery voltage (×0.1V, range 2.0-3.0V)
 * [2]:     Temperature (×1.0°C)
 * [3]:     Pressure MSB (upper byte of 16-bit LE value)
 * [4]:     Pressure LSB (lower byte)
 * [5-10]:  Sensor ID (6 bytes from MAC address)
 *
 * MAC Address structure:
 * [0-1]:   0x37:0x39 (manufacturer TC.TPMS)
 * [2]:     Wheel number (0x01=wheel1, 0x02=wheel2, etc.)
 * [3-5]:   Unique sensor ID (3 bytes)
 * 
 * Formulas:
 * - Pressure: PSI = 0.10223139 * (byte[4] + byte[3]*256) - 14.61232950
 * - Battery: V = byte[1] * 0.1, Pct = min((V - 2.6) * 166.67, 100%)
 * - Temperature: °C = byte[2] * 1.0 (no scaling)
 * - Alarm: (byte[0] & 0x02) != 0
 *
 * @author Artur Jakubowicz
 * @date 20 Nov 2025
 */

#ifndef MAIN_TPMSUTIL_TYPE2_H_
#define MAIN_TPMSUTIL_TYPE2_H_

#include "TPMSSensor.h"  // Base class
#include <string>      // std::string
#include <cstdint>     // uint64_t

/**
 * @class TPMSUtil_Type2
 * @brief TPMS Type 2 sensor data container and parser
 * @details Validates and parses BLE manufacturer data from TC.TPMS Type 2 sensors.
 *          Service UUID: 0xA828
 *          Data length: 11 bytes (5 data + 6 ID bytes from MAC)
 *          Inherits from TPMSSensor for polymorphic handling.
 */
class TPMSUtil_Type2 : public TPMSSensor {
public:
	virtual ~TPMSUtil_Type2();
	
	// ========================================================================
	// Base class interface implementation
	// ========================================================================
	std::string getSensorType() const override { return "Type2"; }
	float getPressurePSI() const override { return m_pressurePSI; }
	float getPressureBar() const override { return m_pressurePSI * 0.0689476f; }  // PSI to BAR conversion
	float getTemperatureC() const override { return m_temperatureC; }
	const std::string& getAddress() const override { return m_address; }
	uint64_t getTimestamp() const override { return m_timestamp; }
	bool getAlert() const override { return m_alarm; }
	uint8_t getBatteryLevel() const override { return m_batteryPercentage; }  // Returns 0-100% for Type 2
	uint8_t getWheelNumber() const override { return m_wheelNumber; }
	const uint8_t* getSensorID() const override { return m_sensorID; }
	
	/**
	 * @brief Check if manufacturer data is from a TPMS Type 2 sensor
	 * @param manufacturerData Raw BLE manufacturer data
	 * @param length Data length
	 * @return true if data matches TPMS Type 2 sensor format (11 bytes)
	 */
	static bool isTPMSSensor_Type2(const uint8_t* data, size_t length);
	
	/**
	 * @brief Parse manufacturer data and create TPMSUtil_Type2 object
	 * @param manufacturerData Raw BLE manufacturer data (11 bytes)
	 * @param address Sensor MAC address
	 * @return Pointer to new TPMSUtil_Type2 object, or nullptr if invalid
	 */
	static TPMSUtil_Type2 *parse(const uint8_t* data, size_t length, const std::string& address);

	/** @brief Get battery voltage in volts */
	float getBatteryVoltage() const { return m_batteryVoltage; }
	
	/** @brief Get battery level as percentage (0-100%) */
	uint8_t getBatteryPercentage() const { return m_batteryPercentage; }
	
	/** @brief Get alarm flag (true if bit 1 of status byte is set) */
	bool getAlarm() const { return m_alarm; }

private:
	/**
	 * @brief Private constructor - use parse() factory method
	 * @param address Sensor MAC address
	 * @param data Raw manufacturer data (11 bytes)
	 */
	TPMSUtil_Type2(const std::string& address, const uint8_t* data);

	/** @brief Parse pressure data from bytes [3-4] */
	void parsePressure();
	
	/** @brief Parse temperature data from byte [2] */
	void parseTemperature();
	
	/** @brief Parse battery level from byte [1] */
	void parseBattery();
	
	/** @brief Parse alarm flag from byte [0] bit 1 */
	void parseStatus();
	
	/** @brief Parse wheel number from MAC address byte [2] */
	void parseWheelNumber(const std::string& address);
	
	/** @brief Parse sensor ID from MAC address bytes [3-5] */
	void parseSensorID(const std::string& address);

	uint8_t m_manufacturerData[11];  ///< Raw manufacturer data
	std::string m_address;           ///< Sensor MAC address
	
	// Private member variables
	uint8_t m_wheelNumber;         ///< Wheel number (1-4)
	uint8_t m_sensorID[3];         ///< Sensor unique ID (3 bytes)
	float m_pressurePSI;           ///< Pressure in PSI
	float m_temperatureC;          ///< Temperature in Celsius
	float m_batteryVoltage;        ///< Battery voltage in volts
	uint8_t m_batteryPercentage;   ///< Battery as percentage (0-100)
	bool m_alarm;                  ///< Alarm flag (bit 1 of status byte)
	uint64_t m_timestamp;          ///< Last update timestamp (ms)
};

#endif /* MAIN_TPMSUTIL_TYPE2_H_ */
