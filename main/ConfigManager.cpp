/**
 * @file ConfigManager.cpp
 * @brief Implementation of JSON-based configuration management with NVS persistence
 * @details Provides persistent configuration storage using ESP32's Non-Volatile Storage
 *          and cJSON library for flexible data serialization
 */

#include "ConfigManager.h"
#include <esp_log.h>   // ESP logging functions
#include <cstring>     // String utilities

/// Log tag for ConfigManager module
static const char* TAG = "ConfigManager";

/**
 * @brief Construct ConfigManager with namespace and key
 * @param namespaceName NVS namespace (partition) to use
 * @param key NVS key name for the JSON configuration string
 */
ConfigManager::ConfigManager(const std::string& namespaceName, const std::string& key)
    : m_isInitialized(false), m_nvsHandle(0), m_namespaceName(namespaceName), 
      m_configKey(key), m_configJson(nullptr) {
}

/**
 * @brief Destructor - cleanup JSON object and close NVS handle
 */
ConfigManager::~ConfigManager() {
    // Free cJSON object if allocated
    if (m_configJson != nullptr) {
        cJSON_Delete(m_configJson);
    }
    // Close NVS handle if open
    if (m_isInitialized && m_nvsHandle != 0) {
        nvs_close(m_nvsHandle);
    }
}

/**
 * @brief Initialize NVS subsystem and load configuration
 * @details Performs:
 *          1. NVS flash initialization (erases if needed)
 *          2. Opens NVS namespace
 *          3. Loads existing config or creates empty JSON object
 * @return true on successful initialization
 */
bool ConfigManager::init() {
    // Initialize NVS flash partition
    esp_err_t err = nvs_flash_init();
    
    // Handle NVS errors that require erasing
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI(TAG, "NVS flash needs to be erased");
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS initialization failed: %s", esp_err_to_name(err));
        return false;
    }
    
    // Open NVS namespace with read/write access
    err = nvs_open(m_namespaceName.c_str(), NVS_READWRITE, &m_nvsHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
        return false;
    }
    
    m_isInitialized = true;
    
    // Load existing configuration or create new empty object
    if (!loadJsonFromNVS()) {
        m_configJson = cJSON_CreateObject();
        ESP_LOGI(TAG, "Created new JSON config");
    }
    
    ESP_LOGI(TAG, "ConfigManager initialized successfully");
    return true;
}

/**
 * @brief Load JSON configuration from NVS
 * @details Reads JSON string from NVS and parses it into cJSON object
 * @return true if loaded or created new config
 */
bool ConfigManager::loadJsonFromNVS() {
    if (!m_isInitialized) return false;
    
    // Query required buffer size
    size_t requiredSize = 0;
    esp_err_t err = nvs_get_str(m_nvsHandle, m_configKey.c_str(), nullptr, &requiredSize);
    
    // If no config exists, create new empty object
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No existing config found");
        m_configJson = cJSON_CreateObject();
        return true;
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get config size: %s", esp_err_to_name(err));
        return false;
    }
    
    // Allocate buffer for JSON string
    char* buffer = (char*)malloc(requiredSize);
    if (buffer == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate memory for config");
        return false;
    }
    
    // Read JSON string from NVS
    err = nvs_get_str(m_nvsHandle, m_configKey.c_str(), buffer, &requiredSize);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get config: %s", esp_err_to_name(err));
        free(buffer);
        return false;
    }
    
    // Delete old JSON object if exists
    if (m_configJson != nullptr) {
        cJSON_Delete(m_configJson);
    }
    
    // Parse JSON string
    m_configJson = cJSON_Parse(buffer);
    free(buffer);
    
    // Handle parse errors
    if (m_configJson == nullptr) {
        ESP_LOGE(TAG, "Failed to parse JSON config");
        m_configJson = cJSON_CreateObject();
        return false;
    }
    
    ESP_LOGI(TAG, "Config loaded from NVS");
    return true;
}

/**
 * @brief Save JSON configuration to NVS
 * @details Serializes cJSON object to string and writes to NVS with commit
 * @return true if saved successfully
 */
bool ConfigManager::saveJsonToNVS() {
    if (!m_isInitialized || m_configJson == nullptr) return false;
    
    // Convert JSON object to formatted string
    char* jsonString = cJSON_Print(m_configJson);
    if (jsonString == nullptr) {
        ESP_LOGE(TAG, "Failed to convert JSON to string");
        return false;
    }
    
    // Write string to NVS
    esp_err_t err = nvs_set_str(m_nvsHandle, m_configKey.c_str(), jsonString);
    free(jsonString);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save config to NVS: %s", esp_err_to_name(err));
        return false;
    }
    
    // Commit changes to flash
    err = nvs_commit(m_nvsHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        return false;
    }
    
    ESP_LOGI(TAG, "Config saved to NVS");
    return true;
}

/**
 * @brief Public wrapper for loading config from NVS
 */
bool ConfigManager::loadConfig() {
    return loadJsonFromNVS();
}

/**
 * @brief Public wrapper for saving config to NVS
 */
bool ConfigManager::saveConfig() {
    return saveJsonToNVS();
}

/**
 * @brief Set integer value in config
 * @param key Configuration key
 * @param value Integer value to store
 * @return true if saved successfully
 */
bool ConfigManager::setInt(const std::string& key, int value) {
    if (m_configJson == nullptr) return false;
    
    // Remove existing item and add new value
    cJSON_DeleteItemFromObject(m_configJson, key.c_str());
    cJSON_AddNumberToObject(m_configJson, key.c_str(), value);
    
    return saveJsonToNVS();
}

/**
 * @brief Set double value in config
 * @param key Configuration key
 * @param value Double value to store
 * @return true if saved successfully
 */
bool ConfigManager::setDouble(const std::string& key, double value) {
    if (!m_isInitialized || !m_configJson) return false;
    
    // Update existing item or add new one
    cJSON* item = cJSON_GetObjectItem(m_configJson, key.c_str());
    if (item) {
        cJSON_SetNumberValue(item, value);
    } else {
        cJSON_AddNumberToObject(m_configJson, key.c_str(), value);
    }
    
    return saveJsonToNVS();
}

/**
 * @brief Set float value in config
 * @param key Configuration key
 * @param value Float value to store
 * @return true if saved successfully
 */
bool ConfigManager::setFloat(const std::string& key, float value) {
    if (!m_isInitialized || !m_configJson) return false;
    
    // Update existing item or add new one (cast to double for cJSON)
    cJSON* item = cJSON_GetObjectItem(m_configJson, key.c_str());
    if (item) {
        cJSON_SetNumberValue(item, static_cast<double>(value));
    } else {
        cJSON_AddNumberToObject(m_configJson, key.c_str(), static_cast<double>(value));
    }
    
    return saveJsonToNVS();
}

/**
 * @brief Set string value in config
 * @param key Configuration key
 * @param value String value to store
 * @return true if saved successfully
 */
bool ConfigManager::setString(const std::string& key, const std::string& value) {
    if (!m_isInitialized || !m_configJson) return false;
    
    // Update existing item or add new one
    cJSON* item = cJSON_GetObjectItem(m_configJson, key.c_str());
    if (item) {
        cJSON_SetValuestring(item, value.c_str());
    } else {
        cJSON_AddStringToObject(m_configJson, key.c_str(), value.c_str());
    }
    
    return saveJsonToNVS();
}

/**
 * @brief Set boolean value in config
 * @param key Configuration key
 * @param value Boolean value to store
 * @return true if saved successfully
 */
bool ConfigManager::setBool(const std::string& key, bool value) {
    if (m_configJson == nullptr) return false;
    
    // Remove existing item and add new value
    cJSON_DeleteItemFromObject(m_configJson, key.c_str());
    cJSON_AddBoolToObject(m_configJson, key.c_str(), value);
    
    return saveJsonToNVS();
}

/**
 * @brief Get integer value from config
 * @param key Configuration key
 * @param value Output parameter for retrieved value
 * @param defaultValue Default value if key not found
 * @return true if successful (even if using default)
 */
bool ConfigManager::getInt(const std::string& key, int& value, int defaultValue) const {
    if (m_configJson == nullptr) {
        value = defaultValue;
        return false;
    }
    
    // Search for key (case-sensitive)
    cJSON* item = cJSON_GetObjectItemCaseSensitive(m_configJson, key.c_str());
    if (item == nullptr || !cJSON_IsNumber(item)) {
        value = defaultValue;
        return true;  // Return true even with default value
    }
    
    value = (int)item->valuedouble;
    return true;
}

/**
 * @brief Get double value from config
 * @param key Configuration key
 * @param value Output parameter for retrieved value
 * @param defaultValue Default value if key not found
 * @return true if key exists, false if using default
 */
bool ConfigManager::getDouble(const std::string& key, double& value, double defaultValue) const {
    if (!m_isInitialized || !m_configJson) {
        value = defaultValue;
        return false;
    }
    
    cJSON* item = cJSON_GetObjectItem(m_configJson, key.c_str());
    if (item && cJSON_IsNumber(item)) {
        value = item->valuedouble;
        return true;
    }
    
    value = defaultValue;
    return false;
}

/**
 * @brief Get float value from config
 * @param key Configuration key
 * @param value Output parameter for retrieved value
 * @param defaultValue Default value if key not found
 * @return true if key exists, false if using default
 */
bool ConfigManager::getFloat(const std::string& key, float& value, float defaultValue) const {
    if (!m_isInitialized || !m_configJson) {
        value = defaultValue;
        return false;
    }
    
    cJSON* item = cJSON_GetObjectItem(m_configJson, key.c_str());
    if (item && cJSON_IsNumber(item)) {
        value = static_cast<float>(item->valuedouble);
        return true;
    }
    
    value = defaultValue;
    return false;
}

/**
 * @brief Get string value from config
 * @param key Configuration key
 * @param value Output parameter for retrieved value
 * @param defaultValue Default value if key not found
 * @return true if key exists, false if using default
 */
bool ConfigManager::getString(const std::string& key, std::string& value, const std::string& defaultValue) const {
    if (!m_isInitialized || !m_configJson) {
        value = defaultValue;
        return false;
    }
    
    cJSON* item = cJSON_GetObjectItem(m_configJson, key.c_str());
    if (item && cJSON_IsString(item)) {
        value = item->valuestring;
        return true;
    }
    
    value = defaultValue;
    return false;
}

/**
 * @brief Get boolean value from config
 * @param key Configuration key
 * @param value Output parameter for retrieved value
 * @param defaultValue Default value if key not found
 * @return true if successful (even if using default)
 */
bool ConfigManager::getBool(const std::string& key, bool& value, bool defaultValue) const {
    if (m_configJson == nullptr) {
        value = defaultValue;
        return false;
    }
    
    cJSON* item = cJSON_GetObjectItemCaseSensitive(m_configJson, key.c_str());
    if (item == nullptr || !cJSON_IsBool(item)) {
        value = defaultValue;
        return true;  // Return true even with default value
    }
    
    value = cJSON_IsTrue(item);
    return true;
}

/**
 * @brief Delete a key from configuration
 * @param key Configuration key to remove
 * @return true if saved successfully
 */
bool ConfigManager::deleteKey(const std::string& key) {
    if (m_configJson == nullptr) return false;
    
    cJSON_DeleteItemFromObject(m_configJson, key.c_str());
    return saveJsonToNVS();
}

/**
 * @brief Erase all configuration data
 * @return true if saved successfully
 */
bool ConfigManager::eraseAll() {
    // Delete old JSON object
    if (m_configJson != nullptr) {
        cJSON_Delete(m_configJson);
    }
    // Create new empty object
    m_configJson = cJSON_CreateObject();
    return saveJsonToNVS();
}

/**
 * @brief Get entire configuration as JSON string
 * @return Formatted JSON string representation
 */
std::string ConfigManager::getJsonString() const {
    if (m_configJson == nullptr) return "{}";
    
    // Convert to formatted JSON string
    char* jsonString = cJSON_Print(m_configJson);
    std::string result(jsonString);
    free(jsonString);
    return result;
}

/**
 * @brief Set entire configuration from JSON string
 * @param jsonString JSON string to parse and store
 * @return true if parsed and saved successfully
 */
bool ConfigManager::setJsonString(const std::string& jsonString) {
    // Parse JSON string
    cJSON* newJson = cJSON_Parse(jsonString.c_str());
    if (newJson == nullptr) {
        ESP_LOGE(TAG, "Failed to parse JSON string");
        return false;
    }
    
    // Delete old JSON object
    if (m_configJson != nullptr) {
        cJSON_Delete(m_configJson);
    }
    m_configJson = newJson;
    
    return saveJsonToNVS();
}