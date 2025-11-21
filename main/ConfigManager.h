#pragma once

#include <string>
#include <nvs_flash.h>
#include <nvs.h>
#include <cJSON.h>

/**
 * @class ConfigManager
 * @brief Manages application configuration storage in NVS using JSON format
 * @details Provides a JSON-based configuration system that persists to
 *          ESP32's Non-Volatile Storage (NVS). All configuration is stored
 *          as a single JSON object, making it easy to backup and restore.
 * 
 * Features:
 * - Type-safe getters and setters for common types
 * - Automatic JSON serialization/deserialization
 * - Persistent storage in NVS flash
 * - Default value support for missing keys
 */
class ConfigManager {
public:
	/**
	 * @brief Constructor
	 * @param namespaceName NVS namespace to use for storage
	 * @param key Key name for the JSON config string in NVS
	 */
	ConfigManager(const std::string& namespaceName = "config", const std::string& key = "config_json");
	~ConfigManager();

	/**
	 * @brief Initialize NVS and load existing configuration
	 * @return true if successful, false otherwise
	 */
	bool init();
	
	/**
	 * @brief Reload configuration from NVS
	 * @return true if successful, false otherwise
	 */
	bool loadConfig();
	
	/**
	 * @brief Save current configuration to NVS
	 * @return true if successful, false otherwise
	 */
	bool saveConfig();
	
	// Setters for different data types (auto-save to NVS)
	bool setInt(const std::string& key, int value);
	bool setDouble(const std::string& key, double value);
	bool setFloat(const std::string& key, float value);
	bool setString(const std::string& key, const std::string& value);
	bool setBool(const std::string& key, bool value);
	
	// Getters for different data types (with default value support)
	bool getInt(const std::string& key, int& value, int defaultValue = 0) const;
	bool getDouble(const std::string& key, double& value, double defaultValue = 0.0) const;
	bool getFloat(const std::string& key, float& value, float defaultValue = 0.0f) const;
	bool getString(const std::string& key, std::string& value, const std::string& defaultValue = "") const;
	bool getBool(const std::string& key, bool& value, bool defaultValue = false) const;
	
	/**
	 * @brief Delete a key from configuration
	 * @param key Key to delete
	 * @return true if successful, false otherwise
	 */
	bool deleteKey(const std::string& key);
	
	/**
	 * @brief Erase all configuration data
	 * @return true if successful, false otherwise
	 */
	bool eraseAll();
	
	/**
	 * @brief Get entire configuration as JSON string
	 * @return JSON string representation of configuration
	 */
	std::string getJsonString() const;
	
	/**
	 * @brief Set entire configuration from JSON string
	 * @param jsonString JSON string to parse and save
	 * @return true if successful, false otherwise
	 */
	bool setJsonString(const std::string& jsonString);
	
	/**
	 * @brief Check if ConfigManager is initialized
	 * @return true if initialized, false otherwise
	 */
	bool isInitialized() const { return m_isInitialized; }

private:
	/**
	 * @brief Load JSON configuration from NVS
	 * @return true if successful, false otherwise
	 */
	bool loadJsonFromNVS();
	
	/**
	 * @brief Save JSON configuration to NVS
	 * @return true if successful, false otherwise
	 */
	bool saveJsonToNVS();

	bool m_isInitialized;           ///< Initialization state flag
	nvs_handle_t m_nvsHandle;       ///< NVS handle for storage operations
	std::string m_namespaceName;    ///< NVS namespace name
	std::string m_configKey;        ///< Key for JSON config in NVS
	cJSON* m_configJson;            ///< cJSON object for configuration data
};