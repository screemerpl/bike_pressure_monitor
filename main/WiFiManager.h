/**
 * @file WiFiManager.h
 * @brief WiFi Access Point manager
 * @details Singleton manager for configuring and running WiFi AP mode.
 *          Used for device configuration web interface.
 */

#pragma once

#include <string>
#include "esp_netif.h"
#include "esp_wifi.h"

/**
 * @class WiFiManager
 * @brief WiFi AP mode controller
 * @details Responsibilities:
 *          - Initialize WiFi in Access Point mode
 *          - Start/stop AP with predefined SSID/password
 *          - Handle client connection/disconnection events
 *          - Provide AP IP address for web server binding
 *          
 *          Default AP settings:
 *          - SSID: "TPMS-Config"
 *          - Password: "tpms1234"
 *          - Channel: 1
 *          - Max connections: 4
 *          - Auth: WPA2-PSK
 */
class WiFiManager {
public:
	/**
	 * @brief Get singleton instance
	 * @return Reference to WiFiManager singleton
	 */
	static WiFiManager &instance();

	/**
	 * @brief Initialize WiFi subsystem
	 * @return true if initialization succeeded
	 * @details Initializes TCP/IP stack, creates event loop, creates WiFi AP netif,
	 *          and registers event handlers. Must be called before start().
	 */
	bool init();

	/**
	 * @brief Start WiFi Access Point
	 * @return true if AP started successfully
	 * @details Configures and starts WiFi AP with predefined SSID/password.
	 *          Logs IP address after successful start.
	 */
	bool start();

	/**
	 * @brief Stop WiFi Access Point
	 * @details Stops WiFi and deinitializes WiFi driver. Safe to call even if not running.
	 */
	void stop();

	/**
	 * @brief Check if AP is currently running
	 * @return true if AP is active
	 */
	bool isRunning() const { return m_isRunning; }

	/**
	 * @brief Get AP IP address
	 * @return IP address string (e.g., "192.168.4.1") or "0.0.0.0" if not initialized
	 */
	std::string getIPAddress() const;

private:
	WiFiManager() = default;
	~WiFiManager();

	WiFiManager(const WiFiManager &) = delete;
	WiFiManager &operator=(const WiFiManager &) = delete;

	/**
	 * @brief WiFi event handler callback
	 * @param arg User context (WiFiManager instance)
	 * @param event_base Event base (WIFI_EVENT)
	 * @param event_id Event ID (WIFI_EVENT_AP_STACONNECTED/STADISCONNECTED)
	 * @param event_data Event-specific data
	 * @details Logs client connection/disconnection events with MAC address and AID
	 */
	static void wifiEventHandler(void *arg, esp_event_base_t event_base,
								 int32_t event_id, void *event_data);

	esp_netif_t *m_netif = nullptr;  ///< WiFi AP network interface
	bool m_isRunning = false;        ///< AP running state

	static constexpr const char *WIFI_SSID = "TPMS-Config";  ///< AP SSID
	static constexpr const char *WIFI_PASS = "tpms1234";     ///< AP password
	static constexpr int WIFI_CHANNEL = 1;                   ///< WiFi channel
	static constexpr int MAX_CONNECTIONS = 4;                ///< Max simultaneous clients
};
