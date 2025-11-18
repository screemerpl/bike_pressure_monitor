#include "ConfigManager.h"
#include <esp_log.h>
#include <cstring>

static const char* TAG = "ConfigManager";

ConfigManager::ConfigManager(const std::string& namespace_name, const std::string& key)
    : isInitialized(false), nvsHandle(0), namespaceName(namespace_name), 
      configKey(key), configJson(nullptr) {
}

ConfigManager::~ConfigManager() {
    if (configJson != nullptr) {
        cJSON_Delete(configJson);
    }
    if (isInitialized && nvsHandle != 0) {
        nvs_close(nvsHandle);
    }
}

bool ConfigManager::init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI(TAG, "NVS flash needs to be erased");
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS initialization failed: %s", esp_err_to_name(err));
        return false;
    }
    
    err = nvs_open(namespaceName.c_str(), NVS_READWRITE, &nvsHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
        return false;
    }
    
    isInitialized = true;
    
    // Załaduj istniejącą konfigurację lub utwórz nową
    if (!loadJsonFromNVS()) {
        configJson = cJSON_CreateObject();
        ESP_LOGI(TAG, "Created new JSON config");
    }
    
    ESP_LOGI(TAG, "ConfigManager initialized successfully");
    return true;
}

bool ConfigManager::loadJsonFromNVS() {
    if (!isInitialized) return false;
    
    size_t requiredSize = 0;
    esp_err_t err = nvs_get_str(nvsHandle, configKey.c_str(), nullptr, &requiredSize);
    
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No existing config found");
        configJson = cJSON_CreateObject();
        return true;
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get config size: %s", esp_err_to_name(err));
        return false;
    }
    
    char* buffer = (char*)malloc(requiredSize);
    if (buffer == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate memory for config");
        return false;
    }
    
    err = nvs_get_str(nvsHandle, configKey.c_str(), buffer, &requiredSize);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get config: %s", esp_err_to_name(err));
        free(buffer);
        return false;
    }
    
    if (configJson != nullptr) {
        cJSON_Delete(configJson);
    }
    
    configJson = cJSON_Parse(buffer);
    free(buffer);
    
    if (configJson == nullptr) {
        ESP_LOGE(TAG, "Failed to parse JSON config");
        configJson = cJSON_CreateObject();
        return false;
    }
    
    ESP_LOGI(TAG, "Config loaded from NVS");
    return true;
}

bool ConfigManager::saveJsonToNVS() {
    if (!isInitialized || configJson == nullptr) return false;
    
    char* jsonString = cJSON_Print(configJson);
    if (jsonString == nullptr) {
        ESP_LOGE(TAG, "Failed to convert JSON to string");
        return false;
    }
    
    esp_err_t err = nvs_set_str(nvsHandle, configKey.c_str(), jsonString);
    free(jsonString);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save config to NVS: %s", esp_err_to_name(err));
        return false;
    }
    
    err = nvs_commit(nvsHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        return false;
    }
    
    ESP_LOGI(TAG, "Config saved to NVS");
    return true;
}

bool ConfigManager::loadConfig() {
    return loadJsonFromNVS();
}

bool ConfigManager::saveConfig() {
    return saveJsonToNVS();
}

bool ConfigManager::setInt(const std::string& key, int value) {
    if (configJson == nullptr) return false;
    
    cJSON_DeleteItemFromObject(configJson, key.c_str());
    cJSON_AddNumberToObject(configJson, key.c_str(), value);
    
    return saveJsonToNVS();
}

bool ConfigManager::setDouble(const std::string& key, double value) {
    if (!isInitialized || !configJson) return false;
    
    cJSON* item = cJSON_GetObjectItem(configJson, key.c_str());
    if (item) {
        cJSON_SetNumberValue(item, value);
    } else {
        cJSON_AddNumberToObject(configJson, key.c_str(), value);
    }
    
    return saveJsonToNVS();
}

bool ConfigManager::setFloat(const std::string& key, float value) {
    if (!isInitialized || !configJson) return false;
    
    cJSON* item = cJSON_GetObjectItem(configJson, key.c_str());
    if (item) {
        cJSON_SetNumberValue(item, static_cast<double>(value));
    } else {
        cJSON_AddNumberToObject(configJson, key.c_str(), static_cast<double>(value));
    }
    
    return saveJsonToNVS();
}

bool ConfigManager::setString(const std::string& key, const std::string& value) {
    if (!isInitialized || !configJson) return false;
    
    cJSON* item = cJSON_GetObjectItem(configJson, key.c_str());
    if (item) {
        cJSON_SetValuestring(item, value.c_str());
    } else {
        cJSON_AddStringToObject(configJson, key.c_str(), value.c_str());
    }
    
    return saveJsonToNVS();
}

bool ConfigManager::setBool(const std::string& key, bool value) {
    if (configJson == nullptr) return false;
    
    cJSON_DeleteItemFromObject(configJson, key.c_str());
    cJSON_AddBoolToObject(configJson, key.c_str(), value);
    
    return saveJsonToNVS();
}

bool ConfigManager::getInt(const std::string& key, int& value, int defaultValue) {
    if (configJson == nullptr) {
        value = defaultValue;
        return false;
    }
    
    cJSON* item = cJSON_GetObjectItemCaseSensitive(configJson, key.c_str());
    if (item == nullptr || !cJSON_IsNumber(item)) {
        value = defaultValue;
        return true;
    }
    
    value = (int)item->valuedouble;
    return true;
}

bool ConfigManager::getDouble(const std::string& key, double& value, double defaultValue) {
    if (!isInitialized || !configJson) {
        value = defaultValue;
        return false;
    }
    
    cJSON* item = cJSON_GetObjectItem(configJson, key.c_str());
    if (item && cJSON_IsNumber(item)) {
        value = item->valuedouble;
        return true;
    }
    
    value = defaultValue;
    return false;
}

bool ConfigManager::getFloat(const std::string& key, float& value, float defaultValue) {
    if (!isInitialized || !configJson) {
        value = defaultValue;
        return false;
    }
    
    cJSON* item = cJSON_GetObjectItem(configJson, key.c_str());
    if (item && cJSON_IsNumber(item)) {
        value = static_cast<float>(item->valuedouble);
        return true;
    }
    
    value = defaultValue;
    return false;
}

bool ConfigManager::getString(const std::string& key, std::string& value, const std::string& defaultValue) {
    if (!isInitialized || !configJson) {
        value = defaultValue;
        return false;
    }
    
    cJSON* item = cJSON_GetObjectItem(configJson, key.c_str());
    if (item && cJSON_IsString(item)) {
        value = item->valuestring;
        return true;
    }
    
    value = defaultValue;
    return false;
}

bool ConfigManager::getBool(const std::string& key, bool& value, bool defaultValue) {
    if (configJson == nullptr) {
        value = defaultValue;
        return false;
    }
    
    cJSON* item = cJSON_GetObjectItemCaseSensitive(configJson, key.c_str());
    if (item == nullptr || !cJSON_IsBool(item)) {
        value = defaultValue;
        return true;
    }
    
    value = cJSON_IsTrue(item);
    return true;
}

bool ConfigManager::deleteKey(const std::string& key) {
    if (configJson == nullptr) return false;
    
    cJSON_DeleteItemFromObject(configJson, key.c_str());
    return saveJsonToNVS();
}

bool ConfigManager::eraseAll() {
    if (configJson != nullptr) {
        cJSON_Delete(configJson);
    }
    configJson = cJSON_CreateObject();
    return saveJsonToNVS();
}

std::string ConfigManager::getJsonString() const {
    if (configJson == nullptr) return "{}";
    
    char* jsonString = cJSON_Print(configJson);
    std::string result(jsonString);
    free(jsonString);
    return result;
}

bool ConfigManager::setJsonString(const std::string& jsonString) {
    cJSON* newJson = cJSON_Parse(jsonString.c_str());
    if (newJson == nullptr) {
        ESP_LOGE(TAG, "Failed to parse JSON string");
        return false;
    }
    
    if (configJson != nullptr) {
        cJSON_Delete(configJson);
    }
    configJson = newJson;
    
    return saveJsonToNVS();
}