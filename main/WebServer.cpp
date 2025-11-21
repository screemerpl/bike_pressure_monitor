/**
 * @file WebServer.cpp
 * @brief HTTP server implementation for web configuration interface
 * @details Implements REST API for device configuration and OTA updates.
 *          Uses chunked transfer with retry logic for WiFi/BLE coexistence.
 */

#include "WebServer.h"
#include "Application.h"
#include "State.h"
#include "TPMSSensor.h"
#include "index_html.h"
#include "esp_log.h"
#include <cstdio>
#include <cstring>

static const char *TAG = "WebServer";

/**
 * @brief Get singleton instance
 * @return Reference to WebServer singleton (static local variable)
 */
WebServer &WebServer::instance() {
	static WebServer server;
	return server;
}

/**
 * @brief Destructor - ensures server is stopped
 * @details Calls stop() to clean up HTTP server resources
 */
WebServer::~WebServer() {
	stop();
}

/**
 * @brief Start HTTP server
 * @return true if server started successfully
 * @details Configures server with:
 *          - Stack size: 12KB (increased for large HTML)
 *          - Max URI handlers: 10 (for all API endpoints)
 *          - LRU purge enabled
 *          - Timeouts: 10 seconds
 *          Registers all URI handlers for root, API, and OTA endpoints
 */
bool WebServer::start() {
	if (m_server) {
		ESP_LOGW(TAG, "Server already running");
		return true;
	}

	ESP_LOGI(TAG, "Starting HTTP server");

	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.stack_size = 12288; // Increased stack for larger HTML
	config.max_uri_handlers = 8; // API handlers
	config.lru_purge_enable = true;
	config.recv_wait_timeout = 10;
	config.send_wait_timeout = 10;

	esp_err_t ret = httpd_start(&m_server, &config);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to start server: %s", esp_err_to_name(ret));
		return false;
	}

	// Register URI handlers
	httpd_uri_t root = {.uri = "/",
						.method = HTTP_GET,
						.handler = handleRoot,
						.user_ctx = nullptr};
	httpd_register_uri_handler(m_server, &root);

	httpd_uri_t api_sensors = {.uri = "/api/sensors",
							   .method = HTTP_GET,
							   .handler = handleGetSensors,
							   .user_ctx = nullptr};
	httpd_register_uri_handler(m_server, &api_sensors);

	httpd_uri_t api_config_get = {.uri = "/api/config",
								  .method = HTTP_GET,
								  .handler = handleGetConfig,
								  .user_ctx = nullptr};
	httpd_register_uri_handler(m_server, &api_config_get);

	httpd_uri_t api_config_post = {.uri = "/api/config",
								  .method = HTTP_POST,
								  .handler = handleSetConfig,
								  .user_ctx = nullptr};
	httpd_register_uri_handler(m_server, &api_config_post);

	httpd_uri_t api_clear = {.uri = "/api/clear",
							 .method = HTTP_POST,
							 .handler = handleClearConfig,
							 .user_ctx = nullptr};
	httpd_register_uri_handler(m_server, &api_clear);

	httpd_uri_t api_restart = {.uri = "/api/restart",
						   .method = HTTP_POST,
						   .handler = handleRestart,
						   .user_ctx = nullptr};
	httpd_register_uri_handler(m_server, &api_restart);

	ESP_LOGI(TAG, "HTTP server started successfully");
	return true;
}

/**
 * @brief Stop HTTP server
 * @details Stops server and releases resources. Safe to call if not running.
 */
void WebServer::stop() {
	if (!m_server) {
		return;
	}

	ESP_LOGI(TAG, "Stopping HTTP server");
	httpd_stop(m_server);
	m_server = nullptr;
}

/**
 * @brief Handle GET / - serve HTML configuration interface
 * @param req HTTP request
 * @return ESP_OK on success
 * @details Sends index_html in small chunks (128 bytes) with 20ms delays
 *          for WiFi/BLE coexistence. Implements retry logic (3 attempts)
 *          for failed chunks. Logs progress and errors.
 */
esp_err_t WebServer::handleRoot(httpd_req_t *req) {
	httpd_resp_set_type(req, "text/html");
	httpd_resp_set_hdr(req, "Connection", "close");
	
	// Send HTML in very small chunks with longer delays
	const char *html = index_html;
	size_t len = strlen(html);
	size_t chunk_size = 128; // Very small chunks
	size_t offset = 0;
	int retry_count = 0;
	const int max_retries = 3;
	
	ESP_LOGI(TAG, "Sending HTML page, total size: %zu bytes", len);
	
	while (offset < len) {
		size_t to_send = (len - offset > chunk_size) ? chunk_size : (len - offset);
		esp_err_t ret = httpd_resp_send_chunk(req, html + offset, to_send);
		
		if (ret != ESP_OK) {
			if (retry_count < max_retries) {
				ESP_LOGW(TAG, "Send failed at offset %zu, retry %d/%d", offset, retry_count + 1, max_retries);
				vTaskDelay(pdMS_TO_TICKS(50)); // Longer wait on error
				retry_count++;
				continue; // Retry same chunk
			} else {
				ESP_LOGE(TAG, "Failed to send chunk at offset %zu after %d retries: %s", 
						 offset, max_retries, esp_err_to_name(ret));
				httpd_resp_send_chunk(req, NULL, 0); // Terminate on error
				return ret;
			}
		}
		
		// Success - move to next chunk
		offset += to_send;
		retry_count = 0;
		
		// Longer delay to allow WiFi/BLE coexistence
		if (offset < len) {
			vTaskDelay(pdMS_TO_TICKS(20));
		}
	}
	
	// Send final empty chunk to finish
	esp_err_t ret = httpd_resp_send_chunk(req, NULL, 0);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to send final chunk: %s", esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG, "HTML page sent successfully");
	}
	return ret;
}

/**
 * @brief Handle GET /api/sensors - get current sensor data
 * @param req HTTP request
 * @return ESP_OK on success
 * @details Calls getSensorsJSON() and sends JSON response
 */
esp_err_t WebServer::handleGetSensors(httpd_req_t *req) {
	std::string json = getSensorsJSON();
	return sendJSON(req, json.c_str());
}

/**
 * @brief Handle GET /api/config - get current configuration
 * @param req HTTP request
 * @return ESP_OK on success
 * @details Calls getConfigJSON() and sends JSON response
 */
esp_err_t WebServer::handleGetConfig(httpd_req_t *req) {
	std::string json = getConfigJSON();
	return sendJSON(req, json.c_str());
}

/**
 * @brief Handle POST /api/config - update configuration
 * @param req HTTP request (JSON body)
 * @return ESP_OK on success
 * @details Parses JSON (simple string search, not robust) and updates:
 *          - front_address: Front sensor MAC address
 *          - rear_address: Rear sensor MAC address
 *          - front_ideal_psi: Target pressure for front tire
 *          - rear_ideal_psi: Target pressure for rear tire
 *          - pressure_unit: "PSI" or "BAR"
 *          Saves all changes to NVS via ConfigManager
 */
esp_err_t WebServer::handleSetConfig(httpd_req_t *req) {
	char content[512];
	int ret = httpd_req_recv(req, content, sizeof(content) - 1);
	if (ret <= 0) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid request");
		return ESP_FAIL;
	}
	content[ret] = '\0';

	ESP_LOGI(TAG, "Received config: %s", content);

	// Parse JSON manually (simple parsing)
	Application &app = Application::instance();
	ConfigManager &config = app.getConfig();

	// Extract values (simple string search, not robust JSON parsing)
	char *ptr;

	// Front address
	ptr = strstr(content, "\"front_address\":\"");
	if (ptr) {
		ptr += 17;
		char *end = strchr(ptr, '"');
		if (end) {
			std::string addr(ptr, end - ptr);
			config.setString("front_address", addr);
			ESP_LOGI(TAG, "Set front_address: %s", addr.c_str());
		}
	}

	// Rear address
	ptr = strstr(content, "\"rear_address\":\"");
	if (ptr) {
		ptr += 16;
		char *end = strchr(ptr, '"');
		if (end) {
			std::string addr(ptr, end - ptr);
			config.setString("rear_address", addr);
			ESP_LOGI(TAG, "Set rear_address: %s", addr.c_str());
		}
	}

	// Front PSI
	ptr = strstr(content, "\"front_ideal_psi\":");
	if (ptr) {
		float psi = atof(ptr + 18);
		config.setFloat("front_ideal_psi", psi);
		ESP_LOGI(TAG, "Set front_ideal_psi: %.1f", psi);
	}

	// Rear PSI
	ptr = strstr(content, "\"rear_ideal_psi\":");
	if (ptr) {
		float psi = atof(ptr + 17);
		config.setFloat("rear_ideal_psi", psi);
		ESP_LOGI(TAG, "Set rear_ideal_psi: %.1f", psi);
	}
	
	// Pressure unit
	ptr = strstr(content, "\"pressure_unit\":\"");
	if (ptr) {
		ptr += 17;
		char *end = strchr(ptr, '"');
		if (end) {
			std::string unit(ptr, end - ptr);
			if (unit == "PSI" || unit == "BAR") {
				config.setString("pressure_unit", unit);
				State::getInstance().setPressureUnit(unit);
				ESP_LOGI(TAG, "Set pressure_unit: %s", unit.c_str());
			}
		}
	}

	const char *response = "{\"status\":\"ok\"}";
	return sendJSON(req, response);
}

/**
 * @brief Handle POST /api/pair - start sensor pairing
 * @param req HTTP request
 * @return ESP_OK on success
 * @details TODO: Not yet implemented - placeholder for future pairing API
 */
esp_err_t WebServer::handlePairSensor(httpd_req_t *req) {
	// TODO: Implement pairing logic
	const char *response = "{\"status\":\"ok\"}";
	return sendJSON(req, response);
}

/**
 * @brief Handle POST /api/clear - clear sensor configuration
 * @param req HTTP request
 * @return ESP_OK on success
 * @details Clears sensor addresses (front_address, rear_address) and
 *          resets ideal PSI to defaults (36.0 front, 42.0 rear)
 */
esp_err_t WebServer::handleClearConfig(httpd_req_t *req) {
	ESP_LOGI(TAG, "Clearing configuration");

	Application &app = Application::instance();
	ConfigManager &config = app.getConfig();

	// Clear sensor addresses
	config.setString("front_address", "");
	config.setString("rear_address", "");

	// Reset to default PSI values
	config.setFloat("front_ideal_psi", 36.0f);
	config.setFloat("rear_ideal_psi", 42.0f);

	ESP_LOGI(TAG, "Configuration cleared - addresses reset, PSI set to defaults");

	const char *response = "{\"status\":\"ok\"}";
	return sendJSON(req, response);
}

/**
 * @brief Handle POST /api/restart - reboot device
 * @param req HTTP request
 * @return ESP_OK on success
 * @details Clears wifi_config_mode flag (returns to normal mode),
 *          sends response, waits 1 second, then calls esp_restart()
 */
esp_err_t WebServer::handleRestart(httpd_req_t *req) {
	Application &app = Application::instance();
	ConfigManager &config = app.getConfig();
	
	// Clear WiFi config mode flag to return to normal operation
	config.setInt("wifi_config_mode", 0);
	ESP_LOGI(TAG, "Cleared WiFi config mode flag - will restart in normal mode");
	
	const char *response = "{\"status\":\"restarting\"}";
	sendJSON(req, response);

	// Restart after 1 second
	vTaskDelay(pdMS_TO_TICKS(1000));
	esp_restart();

	return ESP_OK;
}

/**
 * @brief Build JSON string with all detected sensors
 * @return JSON string
 * @details Iterates State sensor map and formats as JSON array with:
 *          address, pressure_psi, pressure_bar, temperature_c, battery_level
 */
std::string WebServer::getSensorsJSON() {
	State &state = State::getInstance();

	std::string json = "{\"sensors\":[";

	bool first = true;
	for (const auto &pair : state.getData()) {
		if (!first)
			json += ",";
		first = false;

		TPMSSensor *sensor = pair.second;
		char buf[256];
		snprintf(buf, sizeof(buf),
				 "{\"address\":\"%s\",\"pressure\":%.1f,\"temperature\":%.1f,\"battery\":%d}",
				 pair.first.c_str(), sensor->getPressurePSI(),
				 sensor->getTemperatureC(), sensor->getBatteryLevel());
		json += buf;
	}

	json += "]}";
	return json;
}

/**
 * @brief Build JSON string with current configuration
 * @return JSON string
 * @details Reads State singleton and formats as JSON with:
 *          front_address, rear_address, front_ideal_psi, rear_ideal_psi
 */
std::string WebServer::getConfigJSON() {
	State &state = State::getInstance();

	char json[512];
	snprintf(json, sizeof(json),
			 "{\"front_address\":\"%s\",\"rear_address\":\"%s\","
			 "\"front_ideal_psi\":%.1f,\"rear_ideal_psi\":%.1f}",
			 state.getFrontAddress().c_str(), state.getRearAddress().c_str(),
			 state.getFrontIdealPSI(), state.getRearIdealPSI());

	return std::string(json);
}

/**
 * @brief Send JSON response with proper headers
 * @param req HTTP request
 * @param json JSON string to send
 * @return ESP_OK on success
 * @details Sets Content-Type to application/json, adds CORS header,
 *          and sends response with full string length
 */
esp_err_t WebServer::sendJSON(httpd_req_t *req, const char *json) {
	httpd_resp_set_type(req, "application/json");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
	httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

