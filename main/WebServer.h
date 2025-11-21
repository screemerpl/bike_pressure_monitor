/**
 * @file WebServer.h
 * @brief HTTP server for web configuration interface
 * @details Singleton HTTP server providing REST API for:
 *          - Device configuration (sensor addresses, target pressures, units)
 *          - Real-time sensor data viewing
 *          - OTA firmware updates
 *          - System restart
 */

#pragma once

#include "esp_http_server.h"
#include <string>

/**
 * @class WebServer
 * @brief HTTP server for device configuration and OTA updates
 * @details Provides REST API endpoints:
 *          - GET /: Serves HTML configuration interface
 *          - GET /api/sensors: Returns current sensor data (JSON)
 *          - GET /api/config: Returns current configuration (JSON)
 *          - POST /api/config: Updates configuration
 *          - POST /api/clear: Clears sensor pairing
 *          - POST /api/restart: Reboots device
 *          - POST /api/ota/upload: Uploads firmware binary
 *          - GET /api/ota/status: Returns OTA progress
 */
class WebServer {
public:
	/**
	 * @brief Get singleton instance
	 * @return Reference to WebServer singleton
	 */
	static WebServer &instance();

	/**
	 * @brief Start HTTP server
	 * @return true if server started successfully
	 * @details Starts HTTP server on default port (80) and registers all
	 *          URI handlers. Uses increased stack size (12KB) for large HTML.
	 */
	bool start();

	/**
	 * @brief Stop HTTP server
	 * @details Stops server and releases resources. Safe to call if not running.
	 */
	void stop();

	/**
	 * @brief Check if server is running
	 * @return true if server is active
	 */
	bool isRunning() const { return m_server != nullptr; }

private:
	WebServer() = default;
	~WebServer();

	WebServer(const WebServer &) = delete;
	WebServer &operator=(const WebServer &) = delete;

	/**
	 * @brief Handle GET / - serve HTML configuration interface
	 * @param req HTTP request
	 * @return ESP_OK on success
	 * @details Sends index_html in small chunks (128 bytes) with 20ms delays
	 *          for WiFi/BLE coexistence. Implements retry logic for failed chunks.
	 */
	static esp_err_t handleRoot(httpd_req_t *req);
	
	/**
	 * @brief Handle GET /api/sensors - get current sensor data
	 * @param req HTTP request
	 * @return ESP_OK on success
	 * @details Returns JSON with all detected sensors (address, pressure, temp, battery)
	 */
	static esp_err_t handleGetSensors(httpd_req_t *req);
	
	/**
	 * @brief Handle GET /api/config - get current configuration
	 * @param req HTTP request
	 * @return ESP_OK on success
	 * @details Returns JSON with front/rear addresses, ideal PSI values, pressure unit
	 */
	static esp_err_t handleGetConfig(httpd_req_t *req);
	
	/**
	 * @brief Handle POST /api/config - update configuration
	 * @param req HTTP request (JSON body)
	 * @return ESP_OK on success
	 * @details Parses JSON and updates front_address, rear_address, front_ideal_psi,
	 *          rear_ideal_psi, pressure_unit in ConfigManager
	 */
	static esp_err_t handleSetConfig(httpd_req_t *req);
	
	/**
	 * @brief Handle POST /api/pair - start sensor pairing
	 * @param req HTTP request
	 * @return ESP_OK on success
	 * @details TODO: Not yet implemented
	 */
	static esp_err_t handlePairSensor(httpd_req_t *req);
	
	/**
	 * @brief Handle POST /api/clear - clear sensor configuration
	 * @param req HTTP request
	 * @return ESP_OK on success
	 * @details Clears sensor addresses and resets ideal PSI to defaults (36.0/42.0)
	 */
	static esp_err_t handleClearConfig(httpd_req_t *req);
	
	/**
	 * @brief Handle POST /api/restart - reboot device
	 * @param req HTTP request
	 * @return ESP_OK on success
	 * @details Clears wifi_config_mode flag and reboots after 1 second delay
	 */
	static esp_err_t handleRestart(httpd_req_t *req);
	
	/**
	 * @brief Handle POST /api/ota/upload - upload firmware binary
	 * @param req HTTP request (binary body)
	 * @return ESP_OK on success
	 * @details Receives firmware binary, validates ESP32 image format,
	 *          writes to OTA partition, and reboots on success
	 */
	static esp_err_t handleOTAUpload(httpd_req_t *req);
	
	/**
	 * @brief Handle GET /api/ota/status - get OTA progress
	 * @param req HTTP request
	 * @return ESP_OK on success
	 * @details Returns JSON with in_progress flag, progress percentage, and error message
	 */
	static esp_err_t handleOTAStatus(httpd_req_t *req);

	/**
	 * @brief Build JSON string with all detected sensors
	 * @return JSON string
	 * @details Iterates State sensor map and formats as JSON array
	 */
	static std::string getSensorsJSON();
	
	/**
	 * @brief Build JSON string with current configuration
	 * @return JSON string
	 * @details Reads ConfigManager and formats front/rear addresses, PSI values, unit
	 */
	static std::string getConfigJSON();
	
	/**
	 * @brief Send JSON response with proper headers
	 * @param req HTTP request
	 * @param json JSON string to send
	 * @return ESP_OK on success
	 * @details Sets Content-Type to application/json and Connection: close
	 */
	static esp_err_t sendJSON(httpd_req_t *req, const char *json);

	httpd_handle_t m_server = nullptr;  ///< HTTP server handle
	
	static bool s_otaInProgress;       ///< OTA upload in progress flag
	static int s_otaProgress;          ///< OTA progress percentage (0-100)
	static std::string s_otaError;     ///< OTA error message
};
