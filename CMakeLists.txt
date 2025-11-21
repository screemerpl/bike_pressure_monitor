# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

add_definitions(-DLV_CONF_INCLUDE_SIMPLE -DLV_KCONFIG_IGNORE )

# Extract Git version
execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Fallback if git fails
if(NOT GIT_VERSION)
    set(GIT_VERSION "1.0.0-dev")
endif()

# Pass version to compiler
add_definitions(-DGIT_VERSION="${GIT_VERSION}")

message(STATUS "Build version: ${GIT_VERSION}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bike_pressure_monitor)

# Copy SPIFFS files from SquareLine UI/drive to spiffs_data BEFORE creating SPIFFS image
execute_process(
    COMMAND ${CMAKE_COMMAND} -E env python ${CMAKE_SOURCE_DIR}/main/copy_spiffs_files.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/main
    RESULT_VARIABLE COPY_RESULT
)
if(NOT COPY_RESULT EQUAL 0)
    message(WARNING "Failed to copy SPIFFS files")
endif()

# SPIFFS partition image generation
set(SPIFFS_IMAGE_FLASH_IN_PROJECT ON)
spiffs_create_partition_image(storage spiffs_data FLASH_IN_PROJECT)

