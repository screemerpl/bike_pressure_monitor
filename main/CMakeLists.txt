# Automatically find all UI sources (SquareLine Studio generated)
file(GLOB_RECURSE UI_SOURCES 
    "UI/components/*.c"
    "UI/screens/*.c"
    "UI/images/*.c"
    "UI/*.c")

# Automatically find all .cpp files in main directory
file(GLOB APP_SOURCES "*.cpp")

idf_component_register(SRCS ${APP_SOURCES}
                            ${UI_SOURCES}
                     INCLUDE_DIRS "." 
                     REQUIRES esp-nimble-cpp lovyan-gfx json lvgl-custom)




