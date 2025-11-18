#pragma once

#include <string>
#include "esp_netif.h"
#include "esp_wifi.h"

class WiFiManager {
public:
	static WiFiManager &instance();

	// Initialize WiFi in AP mode
	bool init();

	// Start WiFi AP
	bool start();

	// Stop WiFi AP
	void stop();

	// Check if AP is running
	bool isRunning() const { return m_isRunning; }

	// Get AP IP address
	std::string getIPAddress() const;

private:
	WiFiManager() = default;
	~WiFiManager();

	WiFiManager(const WiFiManager &) = delete;
	WiFiManager &operator=(const WiFiManager &) = delete;

	static void wifiEventHandler(void *arg, esp_event_base_t event_base,
								 int32_t event_id, void *event_data);

	esp_netif_t *m_netif = nullptr;
	bool m_isRunning = false;

	static constexpr const char *WIFI_SSID = "TPMS-Config";
	static constexpr const char *WIFI_PASS = "tpms1234";
	static constexpr int WIFI_CHANNEL = 1;
	static constexpr int MAX_CONNECTIONS = 4;
};
