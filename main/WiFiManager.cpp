#include "WiFiManager.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <cstring>

static const char *TAG = "WiFiManager";

WiFiManager &WiFiManager::instance() {
	static WiFiManager manager;
	return manager;
}

WiFiManager::~WiFiManager() {
	stop();
}

bool WiFiManager::init() {
	ESP_LOGI(TAG, "Initializing WiFi Manager");

	// Initialize TCP/IP stack
	ESP_ERROR_CHECK(esp_netif_init());

	// Create default event loop if not already created
	esp_err_t ret = esp_event_loop_create_default();
	if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
		ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
		return false;
	}

	// Create WiFi AP netif
	m_netif = esp_netif_create_default_wifi_ap();
	if (!m_netif) {
		ESP_LOGE(TAG, "Failed to create WiFi AP netif");
		return false;
	}

	// Initialize WiFi with default config
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ret = esp_wifi_init(&cfg);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
		return false;
	}

	// Register event handlers
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
											   &wifiEventHandler, this));

	ESP_LOGI(TAG, "WiFi Manager initialized successfully");
	return true;
}

bool WiFiManager::start() {
	ESP_LOGI(TAG, "Starting WiFi AP: %s", WIFI_SSID);

	// Configure WiFi AP
	wifi_config_t wifi_config = {};
	strlcpy(reinterpret_cast<char *>(wifi_config.ap.ssid), WIFI_SSID,
			sizeof(wifi_config.ap.ssid));
	strlcpy(reinterpret_cast<char *>(wifi_config.ap.password), WIFI_PASS,
			sizeof(wifi_config.ap.password));
	wifi_config.ap.ssid_len = strlen(WIFI_SSID);
	wifi_config.ap.channel = WIFI_CHANNEL;
	wifi_config.ap.max_connection = MAX_CONNECTIONS;
	wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

	if (strlen(WIFI_PASS) == 0) {
		wifi_config.ap.authmode = WIFI_AUTH_OPEN;
	}

	// Set WiFi mode to AP
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

	// Start WiFi
	esp_err_t ret = esp_wifi_start();
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
		return false;
	}

	m_isRunning = true;

	ESP_LOGI(TAG, "WiFi AP started: SSID=%s, Password=%s, IP=%s", WIFI_SSID,
			 WIFI_PASS, getIPAddress().c_str());

	return true;
}

void WiFiManager::stop() {
	if (!m_isRunning) {
		return;
	}

	ESP_LOGI(TAG, "Stopping WiFi AP");

	esp_wifi_stop();
	esp_wifi_deinit();

	m_isRunning = false;
}

std::string WiFiManager::getIPAddress() const {
	if (!m_netif) {
		return "0.0.0.0";
	}

	esp_netif_ip_info_t ip_info;
	esp_netif_get_ip_info(m_netif, &ip_info);

	char ip_str[16];
	snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));

	return std::string(ip_str);
}

void WiFiManager::wifiEventHandler(void *arg, esp_event_base_t event_base,
								   int32_t event_id, void *event_data) {
	if (event_base == WIFI_EVENT) {
		switch (event_id) {
		case WIFI_EVENT_AP_STACONNECTED: {
			wifi_event_ap_staconnected_t *event =
				static_cast<wifi_event_ap_staconnected_t *>(event_data);
			ESP_LOGI(TAG, "Station " MACSTR " connected, AID=%d",
					 MAC2STR(event->mac), event->aid);
			break;
		}
		case WIFI_EVENT_AP_STADISCONNECTED: {
			wifi_event_ap_stadisconnected_t *event =
				static_cast<wifi_event_ap_stadisconnected_t *>(event_data);
			ESP_LOGI(TAG, "Station " MACSTR " disconnected, AID=%d",
					 MAC2STR(event->mac), event->aid);
			break;
		}
		default:
			break;
		}
	}
}
