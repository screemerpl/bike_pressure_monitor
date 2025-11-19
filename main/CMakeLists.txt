# Automatically find all UI sources (SquareLine Studio generated)
# Exclude images/ directory - now using PNG from SPIFFS instead of C arrays
file(GLOB_RECURSE UI_SOURCES 
    "UI/components/*.c"
    "UI/screens/*.c"
    "UI/ui.c"
    "UI/ui_helpers.c")

# Automatically find all .cpp files in main directory
file(GLOB APP_SOURCES "*.cpp")

idf_component_register(SRCS ${APP_SOURCES}
                            ${UI_SOURCES}
                     INCLUDE_DIRS "." 
                     REQUIRES esp-nimble-cpp lovyan-gfx json lvgl-custom esp_http_server esp_netif esp_wifi nvs_flash app_update spiffs)




