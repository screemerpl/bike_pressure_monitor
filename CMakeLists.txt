# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

add_definitions(-DLV_CONF_INCLUDE_SIMPLE -DLV_KCONFIG_IGNORE )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bike_pressure_monitor)

