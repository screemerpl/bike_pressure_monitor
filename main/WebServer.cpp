#include "WebServer.h"
#include "Application.h"
#include "State.h"
#include "index_html.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include <cstdio>
#include <cstring>

static const char *TAG = "WebServer";

// OTA static members
bool WebServer::s_otaInProgress = false;
int WebServer::s_otaProgress = 0;
std::string WebServer::s_otaError = "";

WebServer &WebServer::instance() {
	static WebServer server;
	return server;
}

WebServer::~WebServer() {
	stop();
}

bool WebServer::start() {
	if (m_server) {
		ESP_LOGW(TAG, "Server already running");
		return true;
	}

	ESP_LOGI(TAG, "Starting HTTP server");

	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.stack_size = 12288; // Increased stack for larger HTML
	config.max_uri_handlers = 10; // Increased for OTA handlers
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

	httpd_uri_t api_ota_upload = {.uri = "/api/ota/upload",
								  .method = HTTP_POST,
								  .handler = handleOTAUpload,
								  .user_ctx = nullptr};
	httpd_register_uri_handler(m_server, &api_ota_upload);

	httpd_uri_t api_ota_status = {.uri = "/api/ota/status",
								  .method = HTTP_GET,
								  .handler = handleOTAStatus,
								  .user_ctx = nullptr};
	httpd_register_uri_handler(m_server, &api_ota_status);

	ESP_LOGI(TAG, "HTTP server started successfully");
	return true;
}

void WebServer::stop() {
	if (!m_server) {
		return;
	}

	ESP_LOGI(TAG, "Stopping HTTP server");
	httpd_stop(m_server);
	m_server = nullptr;
}

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

esp_err_t WebServer::handleGetSensors(httpd_req_t *req) {
	std::string json = getSensorsJSON();
	return sendJSON(req, json.c_str());
}

esp_err_t WebServer::handleGetConfig(httpd_req_t *req) {
	std::string json = getConfigJSON();
	return sendJSON(req, json.c_str());
}

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
				State::getInstance().pressureUnit = unit;
				ESP_LOGI(TAG, "Set pressure_unit: %s", unit.c_str());
			}
		}
	}

	const char *response = "{\"status\":\"ok\"}";
	return sendJSON(req, response);
}

esp_err_t WebServer::handlePairSensor(httpd_req_t *req) {
	// TODO: Implement pairing logic
	const char *response = "{\"status\":\"ok\"}";
	return sendJSON(req, response);
}

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

esp_err_t WebServer::handleRestart(httpd_req_t *req) {
	const char *response = "{\"status\":\"restarting\"}";
	sendJSON(req, response);

	// Restart after 1 second
	vTaskDelay(pdMS_TO_TICKS(1000));
	esp_restart();

	return ESP_OK;
}

std::string WebServer::getSensorsJSON() {
	State &state = State::getInstance();

	std::string json = "{\"sensors\":[";

	bool first = true;
	for (const auto &pair : state.data) {
		if (!first)
			json += ",";
		first = false;

		TPMSUtil *sensor = pair.second;
		char buf[256];
		snprintf(buf, sizeof(buf),
				 "{\"address\":\"%s\",\"pressure\":%.1f,\"temperature\":%.1f,\"battery\":%d}",
				 pair.first.c_str(), sensor->pressurePSI,
				 sensor->temperatureC, sensor->batteryLevel);
		json += buf;
	}

	json += "]}";
	return json;
}

std::string WebServer::getConfigJSON() {
	Application &app = Application::instance();
	State &state = State::getInstance();

	char json[512];
	snprintf(json, sizeof(json),
			 "{\"front_address\":\"%s\",\"rear_address\":\"%s\","
			 "\"front_ideal_psi\":%.1f,\"rear_ideal_psi\":%.1f}",
			 state.frontAddress.c_str(), state.rearAddress.c_str(),
			 state.frontIdealPSI, state.rearIdealPSI);

	return std::string(json);
}

esp_err_t WebServer::sendJSON(httpd_req_t *req, const char *json) {
	httpd_resp_set_type(req, "application/json");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
	httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

esp_err_t WebServer::handleOTAUpload(httpd_req_t *req) {
	ESP_LOGI(TAG, "OTA upload started");
	
	s_otaInProgress = true;
	s_otaProgress = 0;
	s_otaError = "";
	
	esp_ota_handle_t ota_handle;
	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
	
	if (update_partition == NULL) {
		ESP_LOGE(TAG, "No OTA partition found");
		s_otaError = "No OTA partition available";
		s_otaInProgress = false;
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition");
		return ESP_FAIL;
	}
	
	ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%lx", 
			 update_partition->subtype, update_partition->address);
	
	esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
		s_otaError = "OTA begin failed";
		s_otaInProgress = false;
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA begin failed");
		return ESP_FAIL;
	}
	
	char buf[1024];
	int received;
	int total_received = 0;
	int content_length = req->content_len;
	
	ESP_LOGI(TAG, "Expected firmware size: %d bytes", content_length);
	
	while (total_received < content_length) {
		received = httpd_req_recv(req, buf, sizeof(buf));
		
		if (received <= 0) {
			if (received == HTTPD_SOCK_ERR_TIMEOUT) {
				continue;
			}
			ESP_LOGE(TAG, "File receive failed");
			esp_ota_abort(ota_handle);
			s_otaError = "File receive failed";
			s_otaInProgress = false;
			return ESP_FAIL;
		}
		
		err = esp_ota_write(ota_handle, buf, received);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
			esp_ota_abort(ota_handle);
			s_otaError = "OTA write failed";
			s_otaInProgress = false;
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA write failed");
			return ESP_FAIL;
		}
		
		total_received += received;
		s_otaProgress = (total_received * 100) / content_length;
		
		if (s_otaProgress % 10 == 0) {
			ESP_LOGI(TAG, "OTA progress: %d%%", s_otaProgress);
		}
	}
	
	err = esp_ota_end(ota_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
		s_otaError = "OTA end failed";
		s_otaInProgress = false;
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA end failed");
		return ESP_FAIL;
	}
	
	err = esp_ota_set_boot_partition(update_partition);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
		s_otaError = "Failed to set boot partition";
		s_otaInProgress = false;
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Set boot partition failed");
		return ESP_FAIL;
	}
	
	s_otaProgress = 100;
	s_otaInProgress = false;
	ESP_LOGI(TAG, "OTA update successful. Firmware size: %d bytes", total_received);
	
	const char *response = "{\"status\":\"success\",\"message\":\"OTA update completed. Device will restart.\"}";
	sendJSON(req, response);
	
	// Restart after 2 seconds
	vTaskDelay(pdMS_TO_TICKS(2000));
	esp_restart();
	
	return ESP_OK;
}

esp_err_t WebServer::handleOTAStatus(httpd_req_t *req) {
	char json[256];
	snprintf(json, sizeof(json),
			 "{\"in_progress\":%s,\"progress\":%d,\"error\":\"%s\"}",
			 s_otaInProgress ? "true" : "false",
			 s_otaProgress,
			 s_otaError.c_str());
	
	return sendJSON(req, json);
}

