# Automatically find all UI sources (SquareLine Studio generated)
file(GLOB_RECURSE UI_SOURCES 
    "UI/components/*.c"
    "UI/screens/*.c"
    "UI/images/*.c"
    "UI/*.c"
    "UI/images/ui_img_942102620.c")

# Automatically find all .cpp files in main directory
file(GLOB APP_SOURCES "*.cpp")
file(GLOB APP_C_SOURCES "*.c")

idf_component_register(SRCS ${APP_SOURCES} ${APP_C_SOURCES}
                            ${UI_SOURCES}
                     INCLUDE_DIRS "." 
                     REQUIRES esp-nimble-cpp lovyan-gfx json lvgl-custom esp_http_server esp_netif esp_wifi nvs_flash app_update spiffs)


                     
# Fix LVGL includes before building this component
idf_build_get_property(python PYTHON)
add_custom_target(fix_lvgl_includes
    COMMAND ${python} ${CMAKE_CURRENT_SOURCE_DIR}/fix_lvgl_includes.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Fixing LVGL includes in SquareLine generated files..."
    VERBATIM
)
add_dependencies(${COMPONENT_LIB} fix_lvgl_includes)




