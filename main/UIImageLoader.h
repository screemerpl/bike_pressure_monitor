/**
 * @file UIImageLoader.h
 * @brief Wrapper for loading/freeing UI images from SPIFFS
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Load splash screen images with logging
 */
void ui_load_splash_images_wrapper(void);

/**
 * @brief Load main screen images with logging
 */
void ui_load_main_images_wrapper(void);

/**
 * @brief Free splash screen images to reclaim memory
 */
void ui_free_splash_images_wrapper(void);

#ifdef __cplusplus
}
#endif
