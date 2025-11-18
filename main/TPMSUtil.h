/*
 * TPMSUtil.h
 *
 *  Created on: 16 Nov 2025
 *      Author: Artur Jakubowicz
 */

#ifndef MAIN_TPMSUTIL_H_
#define MAIN_TPMSUTIL_H_

#include <string>
#include <cstdint>

class TPMSUtil {

public:
  virtual ~TPMSUtil();
  static bool isTPMSSensor(std::string manufacturerData);
  static TPMSUtil *parse(std::string manufacturerData, std::string address);

  char identifier[3];
  char sensorNumber;
  float pressurePSI;
  float temperatureC;
  char batteryLevel;
  bool alert;
  uint64_t timestamp;
private:
  TPMSUtil(std::string address, std::string manufacturerData);

  long getLongValue(int index);
  void parsePressure();
  void parseTemperature();
  void parseID();
  void parseOther();

  unsigned char manufacturerData[18];
  std::string address;
};

#endif /* MAIN_TPMSUTIL_H_ */
