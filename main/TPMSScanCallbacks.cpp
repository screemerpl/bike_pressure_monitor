#include "TPMSScanCallbacks.h"
#include "State.h"
#include "TPMSUtil.h"
#include <NimBLEDevice.h>
#include <cstdio>
#include <string>

void TPMSScanCallbacks::onDiscovered(
    const NimBLEAdvertisedDevice *advertisedDevice) {

    std::string manufacturerData = advertisedDevice->getManufacturerData();

    if (TPMSUtil::isTPMSSensor(manufacturerData)) {
        TPMSUtil *sensor = TPMSUtil::parse(
            manufacturerData, advertisedDevice->getAddress().toString());

		// Convert timestamp from microseconds to hh:mm:ss
		uint64_t total_seconds = sensor->timestamp / 1000ULL;
		int hours = (total_seconds / 3600) % 24;
		int minutes = (total_seconds / 60) % 60;
		int seconds = total_seconds % 60;

		printf("[%02d:%02d:%02d] TPMS Sensor found at address %s  id: 0x%02x%02x%02x, wheel "
               "index: %d, pressure: %.1f PSI, temperature: %.1f C, battery: "
               "%d%%, "
               "alert: %d\n",
			   hours, minutes, seconds,
               advertisedDevice->getAddress().toString().c_str(),
               sensor->identifier[0], sensor->identifier[1],
               sensor->identifier[2], sensor->sensorNumber, sensor->pressurePSI,
               sensor->temperatureC, sensor->batteryLevel, sensor->alert);
        
        State& state = State::getInstance();
        std::string address = advertisedDevice->getAddress().toString();
        
        if (!state.data.contains(address)) {
            state.data[address] = sensor;
        } else {
           TPMSUtil *existingSensor = state.data[address];
            delete existingSensor;
           state.data[address] = sensor;
        }

        // Do NOT delete sensor - it's now owned by state.data
    }
}

void TPMSScanCallbacks::onResult(
	const NimBLEAdvertisedDevice *advertisedDevice) {
	// Not used - active scanning handled in onDiscovered
}

void TPMSScanCallbacks::onScanEnd(const NimBLEScanResults &results,
								  int reason) {
	//   printf("BLE scan ended (reason = %d), restarting...\n", reason);
	NimBLEDevice::getScan()->start(SCAN_TIME_MS, false, true);
}