/*
 * TPMSUtil.cpp
 *
 *  Created on: 16 Nov 2025
 *      Author: Artur Jakubowicz
 */

#include "TPMSUtil.h"
#include "stdlib.h"
#include "string.h"
#include <esp_timer.h>

// example 0x00 01 81 ea ca 20 04 10 23 06 00 00 1f 0b 00 00 09 00

TPMSUtil::TPMSUtil(std::string address, std::string manufacturerData) {
  // set values:
  this->address = address;
  memcpy(this->manufacturerData, manufacturerData.c_str(), 18);
  this->parseID();
  this->parsePressure();
  this->parseTemperature();
  this->parseOther();
  this->timestamp = esp_timer_get_time() / 1000;;
}
TPMSUtil::~TPMSUtil() {
  // TODO Auto-generated destructor stub
}

bool TPMSUtil::isTPMSSensor(std::string manufacturerData) {
  unsigned char *dataP = (unsigned char *)manufacturerData.c_str();
  if (manufacturerData.size() != 18)
    return false;
  if (dataP[0] != 0x00 || dataP[1] != 0x01)
    return false;
  if (dataP[3] != 0xea || dataP[4] != 0xca)
    return false;
  if (dataP[2] < 0x80)
    return false;

  return true;
}

TPMSUtil *TPMSUtil::parse(std::string manufacturerData, std::string address) {
  return TPMSUtil::isTPMSSensor(manufacturerData)
             ? new TPMSUtil(address, manufacturerData)
             : nullptr;
}

long TPMSUtil::getLongValue(int index) {
  long result = this->manufacturerData[index] |
                (long)(this->manufacturerData[index + 1]) << 8 |
                (long)(this->manufacturerData[index + 2]) << 16 |
                (long)this->manufacturerData[index + 3] << 24;
  return result;
}

void TPMSUtil::parseID() {
  memcpy(this->identifier, this->manufacturerData + 5, 3);
  this->sensorNumber = this->manufacturerData[2] - 0x80;
}

void TPMSUtil::parsePressure() {
  long result = this->getLongValue(8);
  float kPA = result / 1000.0f;
  this->pressurePSI = kPA * 0.14503773773020923f;
  this->pressureBar = kPA / 100.0f;
}

void TPMSUtil::parseTemperature() {
  long result = this->getLongValue(12);
  float celsius = result / 100.0;
  this->temperatureC = celsius;
}

void TPMSUtil::parseOther() {
  this->batteryLevel = manufacturerData[16];
  if (manufacturerData[17] == 1)
    this->alert = true;
  else
    this->alert = false;
}
