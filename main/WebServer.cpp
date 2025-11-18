#include "WebServer.h"
#include "Application.h"
#include "State.h"
#include "index_html.h"
#include "esp_log.h"
#include <cstdio>
#include <cstring>

static const char *TAG = "WebServer";

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
	config.stack_size = 8192;
	config.max_uri_handlers = 8;
	config.lru_purge_enable = true;

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
	
	// Send HTML in chunks to avoid buffer overflow
	const char *html = index_html;
	size_t len = strlen(html);
	size_t chunk_size = 1024;
	size_t offset = 0;
	
	while (offset < len) {
		size_t to_send = (len - offset > chunk_size) ? chunk_size : (len - offset);
		esp_err_t ret = httpd_resp_send_chunk(req, html + offset, to_send);
		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "Failed to send chunk: %s", esp_err_to_name(ret));
			return ret;
		}
		offset += to_send;
	}
	
	// Send final empty chunk to finish
	httpd_resp_send_chunk(req, NULL, 0);
	return ESP_OK;
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
	ConfigManager &config = app.getConfig();
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
