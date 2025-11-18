#pragma once

#include <NimBLEDevice.h>

class TPMSScanCallbacks : public NimBLEScanCallbacks {
public:
    void onDiscovered(const NimBLEAdvertisedDevice *advertisedDevice) override;
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override;
    void onScanEnd(const NimBLEScanResults &results, int reason) override;

private:
    static constexpr uint32_t SCAN_TIME_MS = 1000;
};