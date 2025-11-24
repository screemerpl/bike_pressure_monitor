/**
 * @file TPMSSensor.h
 * @brief Base class for TPMS sensor data (Type 1 and Type 2)
 * @details Provides common interface for both TPMS sensor types,
 *          enabling polymorphic handling in State map and UI.
 */

#ifndef MAIN_TPMSSENSOR_H_
#define MAIN_TPMSSENSOR_H_

#include <string>      // std::string
#include <cstdint>     // uint64_t, uint8_t

/**
 * @class TPMSSensor
 * @brief Abstract base class for TPMS sensor data
 * @details Defines common interface for both Type 1 and Type 2 sensors.
 *          Derived classes: TPMSUtil (Type 1), TPMSUtil_Type2 (Type 2)
 */
class TPMSSensor {
public:
	virtual ~TPMSSensor() = default;
	
	/**
	 * @brief Get sensor type identifier
	 * @return "Type1" for TPMSUtil, "Type2" for TPMSUtil_Type2
	 */
	virtual std::string getSensorType() const = 0;
	
	/**
	 * @brief Get tire pressure in PSI
	 */
	virtual float getPressurePSI() const = 0;
	
	/**
	 * @brief Get tire pressure in BAR
	 * @details For Type 1: returns pressure in BAR (calculated from PSI)
	 *          For Type 2: returns PSI * 0.0689476 (PSI to BAR conversion)
	 */
	virtual float getPressureBar() const = 0;
	
	/**
	 * @brief Get tire temperature in Celsius
	 */
	virtual float getTemperatureC() const = 0;
	
	/**
	 * @brief Get sensor MAC address
	 */
	virtual const std::string& getAddress() const = 0;
	
	/**
	 * @brief Get timestamp of last update (milliseconds since boot)
	 */
	virtual uint64_t getTimestamp() const = 0;
	
	/**
	 * @brief Get alarm/alert flag
	 */
	virtual bool getAlert() const = 0;
	
	/**
	 * @brief Get battery level (0-100% or 0-255)
	 * @details Type 1: returns raw battery level (0-255)
	 *          Type 2: returns battery percentage (0-100)
	 */
	virtual uint8_t getBatteryLevel() const = 0;
	
	/**
	 * @brief Get wheel/sensor number
	 * @return Wheel number (1-4) or sensor index
	 */
	virtual uint8_t getWheelNumber() const = 0;
	
	/**
	 * @brief Get 3-byte sensor identifier
	 * @return Pointer to 3-byte ID array
	 */
	virtual const uint8_t* getSensorID() const = 0;
};

#endif /* MAIN_TPMSSENSOR_H_ */
