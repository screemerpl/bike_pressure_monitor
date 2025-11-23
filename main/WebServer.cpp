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
#include "UI/ui.h"
#include "UI/ui_themes.h"
#include "PairController.h"
#include "esp_log.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <cstdlib>
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "lvgl.h"

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

	// Diagnostic: log free heap before starting server
	size_t free_heap_before = esp_get_free_heap_size();
	size_t esp8_free_before = heap_caps_get_free_size(MALLOC_CAP_8BIT);
	ESP_LOGI(TAG, "HTTP server start: free_heap=%u bytes, free_8bit=%u bytes, stack_size=%u",
			 static_cast<unsigned int>(free_heap_before),
			 static_cast<unsigned int>(esp8_free_before),
			 static_cast<unsigned int>(config.stack_size));

	esp_err_t ret = httpd_start(&m_server, &config);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to start server: %s", esp_err_to_name(ret));
		// Provide helpful guidance: if task creation failed, it may be due to insufficient
		// contiguous heap to allocate the HTTPD task; try again with a smaller stack size.
		if (ret == ESP_ERR_HTTPD_TASK) {
			ESP_LOGW(TAG, "ESP_ERR_HTTPD_TASK - trying fallback with smaller stack size (4096)");
			config.stack_size = 4096; // fallback to 4 KB stack
			esp_err_t retry = httpd_start(&m_server, &config);
			if (retry == ESP_OK) {
				size_t free_heap_after = esp_get_free_heap_size();
				size_t esp8_free_after = heap_caps_get_free_size(MALLOC_CAP_8BIT);
				ESP_LOGI(TAG, "HTTP server started after fallback - free_heap=%u, free_8bit=%u",
						 static_cast<unsigned int>(free_heap_after),
						 static_cast<unsigned int>(esp8_free_after));
			} else {
				ESP_LOGE(TAG, "Fallback attempt failed: %s", esp_err_to_name(retry));
				return false;
			}
		} else {
			return false;
		}
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
 *          - mode: 0 (Motorcycle) or 1 (Car)
 *          - addresses: array of up to 4 MAC address strings
 *          - ideal_psi: array of float PSI values corresponding to addresses
 *          - pressure_unit: "PSI" or "BAR"
 *          Saves all changes to NVS via ConfigManager (keys: sensor_address_x, sensor_ideal_psi_x)
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
	// Extract values (simple JSON parsing without a full JSON parser)
	char *ptr;

	// Mode: supports either integer or string name
	int appMode = MODE_BIKE;
	ptr = strstr(content, "\"mode\":");
	if (ptr) {
		ptr += 7;
		// Skip whitespace
		while (*ptr == ' ' || *ptr == '\t') ptr++;
		if (*ptr == '"') {
			ptr++;
			char *end = strchr(ptr, '"');
			if (end) {
				std::string m(ptr, end - ptr);
				if (m == "car" || m == "MODE_CAR" || m == "1") {
					appMode = MODE_CAR;
				} else {
					appMode = MODE_BIKE;
				}
			}
		} else {
			// numeric
			appMode = atoi(ptr);
		}
	}
	// Persist app mode
	config.setInt("app_mode", appMode);
	State::getInstance().setMode(appMode);
	ESP_LOGI(TAG, "Set app_mode: %d", appMode);

	// Parse sensor addresses: expect JSON array: "addresses":["aa:bb","cc:dd",...]
	std::vector<std::string> addresses;
	ptr = strstr(content, "\"addresses\":");
	if (ptr) {
		ptr = strchr(ptr, '[');
		if (ptr) {
			ptr++;
			while (*ptr && *ptr != ']') {
				// Find the next quote
				char *start = strchr(ptr, '"');
				if (!start) break;
				start++;
				char *end = strchr(start, '"');
				if (!end) break;
				addresses.emplace_back(start, end - start);
				ptr = end + 1;
				// Move past comma
				char *comma = strchr(ptr, ',');
				if (!comma) break;
				ptr = comma + 1;
			}
		}
	}

	// Parse ideal pressures similarly: "ideal_psi":[36.0,42.0,...]
	std::vector<float> idealPsi;
	ptr = strstr(content, "\"ideal_psi\":");
	if (ptr) {
		ptr = strchr(ptr, '[');
		if (ptr) {
			ptr++;
			while (*ptr && *ptr != ']') {
				while (*ptr == ' ' || *ptr == '\t' || *ptr == ',') ptr++;
				char *end = ptr;
				// read number
				while (*end && *end != ',' && *end != ']') end++;
				char temp[32] = {0};
				size_t len = end - ptr;
				if (len >= sizeof(temp)) len = sizeof(temp) - 1;
				strncpy(temp, ptr, len);
				idealPsi.push_back(static_cast<float>(atof(temp)));
				ptr = end;
				if (*ptr == ',') ptr++;
			}
		}
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

			// UI Theme (numeric index)
			ptr = strstr(content, "\"ui_theme\":");
			if (!ptr) ptr = strstr(content, "\"theme\":");
			if (ptr) {
				if (strncmp(ptr, "\"ui_theme\":", 11) == 0) ptr += 11;
				else if (strncmp(ptr, "\"theme\":", 8) == 0) ptr += 8;
				// Skip whitespace
				while (*ptr == ' ' || *ptr == '\t') ptr++;
				int theme = atoi(ptr);
				if (theme < UI_THEME_DEFAULT) theme = UI_THEME_DEFAULT;
				if (theme > UI_THEME_HYBRID) theme = UI_THEME_HYBRID;
				config.setInt("ui_theme", theme);
				State::getInstance().setUITheme(theme);
				ESP_LOGI(TAG, "Set ui_theme: %d", theme);
				// Apply theme asynchronously on LVGL task
				int *pTheme = (int*)malloc(sizeof(int));
				if (pTheme) {
					*pTheme = theme;
					lv_async_call([](void *arg){
						int t = *((int*)arg);
						ui_theme_set((uint8_t)t);
						free(arg);
					}, pTheme);
				}
			}
		}
	}

	// Now persist addresses and ideal pressures in the keys that Application.cpp expects
	if (appMode == MODE_BIKE) {
		// Bike: keys sensor_address_0 and sensor_address_1
		for (int i = 0; i < 2; ++i) {
			std::string key = "sensor_address_" + std::to_string(i);
			std::string value = (i < (int)addresses.size()) ? addresses[i] : std::string("");
			config.setString(key, value);
			State::getInstance().setAddress(i, value);
			ESP_LOGI(TAG, "Set %s: %s", key.c_str(), value.c_str());
		}
		// Ideal pressures for two sensors
		for (int i = 0; i < 2; ++i) {
			std::string key = "sensor_ideal_psi_" + std::to_string(i);
			float psi = (i < (int)idealPsi.size()) ? idealPsi[i] : State::getInstance().getIdealPSI(i);
			config.setFloat(key, psi);
			State::getInstance().setIdealPSI(i, psi);
			ESP_LOGI(TAG, "Set %s: %.1f", key.c_str(), psi);
		}
	} else {
		// Car: Application.cpp loads keys sensor_address_1..4 and sensor_ideal_psi_1..4
		for (int i = 0; i < 4; ++i) {
			std::string key = "sensor_address_" + std::to_string(i + 1);
			std::string value = (i < (int)addresses.size()) ? addresses[i] : std::string("");
			config.setString(key, value);
			State::getInstance().setAddress(i, value);
			ESP_LOGI(TAG, "Set %s: %s", key.c_str(), value.c_str());
		}
		for (int i = 0; i < 4; ++i) {
			std::string key = "sensor_ideal_psi_" + std::to_string(i + 1);
			float psi = (i < (int)idealPsi.size()) ? idealPsi[i] : State::getInstance().getIdealPSI(i);
			config.setFloat(key, psi);
			State::getInstance().setIdealPSI(i, psi);
			ESP_LOGI(TAG, "Set %s: %.1f", key.c_str(), psi);
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
	// Start pairing via PairController
	ESP_LOGI(TAG, "Pairing requested via API");
	PairController &pc = PairController::instance();
	pc.init();
	const char *response = "{\"status\":\"pairing_started\"}";
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

	// Clear sensor addresses (bike and car keys) and legacy front/rear keys
	for (int i = 0; i < 4; ++i) {
		std::string key0 = "sensor_address_" + std::to_string(i);
		config.setString(key0, "");
		std::string key1 = "sensor_address_" + std::to_string(i + 1);
		config.setString(key1, "");
	}
	// Legacy keys removed; we persist to sensor_address_* only

	// Reset to default PSI values for all 4 positions
	config.setFloat("sensor_ideal_psi_0", 36.0f);
	config.setFloat("sensor_ideal_psi_1", 42.0f);
	config.setFloat("sensor_ideal_psi_2", 36.0f);
	config.setFloat("sensor_ideal_psi_3", 42.0f);

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
 *          mode, addresses[] (4), ideal_psi[] (4), pressure_unit
 */
std::string WebServer::getConfigJSON() {
	State &state = State::getInstance();
	// Return JSON with app mode and addresses/ideal_psi arrays
	char json[1024];
	int mode = state.getMode();
	// Build addresses JSON array with up to 4 sensor addresses (empty strings allowed)
	const std::string &a0 = state.getAddress(0);
	const std::string &a1 = state.getAddress(1);
	const std::string &a2 = state.getAddress(2);
	const std::string &a3 = state.getAddress(3);
	float p0 = state.getIdealPSI(0);
	float p1 = state.getIdealPSI(1);
	float p2 = state.getIdealPSI(2);
	float p3 = state.getIdealPSI(3);

	int theme = state.getUITheme();
	snprintf(json, sizeof(json),
			 "{\"mode\":%d,\"addresses\":[\"%s\",\"%s\",\"%s\",\"%s\"],"
			 "\"ideal_psi\":[%.1f,%.1f,%.1f,%.1f],\"pressure_unit\":\"%s\",\"ui_theme\":%d}",
			 mode,
			 a0.c_str(), a1.c_str(), a2.c_str(), a3.c_str(),
			 p0, p1, p2, p3, state.getPressureUnit().c_str(), theme);

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

