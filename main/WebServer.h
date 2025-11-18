#pragma once

#include "esp_http_server.h"
#include <string>

class WebServer {
public:
	static WebServer &instance();

	// Start HTTP server
	bool start();

	// Stop HTTP server
	void stop();

	// Check if server is running
	bool isRunning() const { return m_server != nullptr; }

private:
	WebServer() = default;
	~WebServer();

	WebServer(const WebServer &) = delete;
	WebServer &operator=(const WebServer &) = delete;

	// HTTP handlers
	static esp_err_t handleRoot(httpd_req_t *req);
	static esp_err_t handleGetSensors(httpd_req_t *req);
	static esp_err_t handleGetConfig(httpd_req_t *req);
	static esp_err_t handleSetConfig(httpd_req_t *req);
	static esp_err_t handlePairSensor(httpd_req_t *req);
	static esp_err_t handleClearConfig(httpd_req_t *req);
	static esp_err_t handleRestart(httpd_req_t *req);

	// Helper functions
	static std::string getSensorsJSON();
	static std::string getConfigJSON();
	static esp_err_t sendJSON(httpd_req_t *req, const char *json);

	httpd_handle_t m_server = nullptr;
};
