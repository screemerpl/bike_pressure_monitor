#pragma once

#include <string>
#include <nvs_flash.h>
#include <nvs.h>
#include <cJSON.h>

class ConfigManager {
private:
    bool isInitialized;
    nvs_handle_t nvsHandle;
    std::string namespaceName;
    std::string configKey;
    cJSON* configJson;

    // Wewnętrzne metody
    bool loadJsonFromNVS();
    bool saveJsonToNVS();

public:
    ConfigManager(const std::string& namespace_name = "config", const std::string& key = "config_json");
    ~ConfigManager();

    // Inicjalizacja NVS
    bool init();
    
    // Operacje na całej konfiguracji JSON
    bool loadConfig();
    bool saveConfig();
    
    // Settery dla różnych typów
    bool setInt(const std::string& key, int value);
    bool setDouble(const std::string& key, double value);
    bool setFloat(const std::string& key, float value);
    bool setString(const std::string& key, const std::string& value);
    bool setBool(const std::string& key, bool value);
    
    // Gettery dla różnych typów
    bool getInt(const std::string& key, int& value, int defaultValue = 0);
    bool getDouble(const std::string& key, double& value, double defaultValue = 0.0);
    bool getFloat(const std::string& key, float& value, float defaultValue = 0.0f);
    bool getString(const std::string& key, std::string& value, const std::string& defaultValue = "");
    bool getBool(const std::string& key, bool& value, bool defaultValue = false);
    
    // Utility
    bool deleteKey(const std::string& key);
    bool eraseAll();
    std::string getJsonString() const;
    bool setJsonString(const std::string& jsonString);
    bool isInitialized_check() const { return isInitialized; }
};