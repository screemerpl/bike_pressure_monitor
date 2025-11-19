#pragma once

#include "LGFX_driver.h"

/**
 * @class DisplayManager
 * @brief Manages LCD display initialization and rendering
 * @details Singleton class that handles:
 *          - TFT display initialization via LovyanGFX
 *          - LVGL integration and buffer management
 *          - PWM backlight control
 *          - Display flush operations for LVGL
 */
class DisplayManager {
public:
	/**
	 * @brief Initialize display hardware and LVGL
	 */
	void init();
	
	/**
	 * @brief Flush pixel data to display
	 * @param disp LVGL display object
	 * @param area Screen area to update
	 * @param colorP Pixel data buffer
	 */
	void flushScreen(lv_display_t *disp, const lv_area_t *area,
					 uint8_t *colorP);
	
	/**
	 * @brief Get singleton instance
	 * @return Pointer to DisplayManager instance
	 */
	static DisplayManager *instance() {
		if (s_instance == nullptr) {
			s_instance = new DisplayManager();
		}
		return s_instance;
	}
	
	/**
	 * @brief Set display backlight brightness
	 * @param brightness Brightness percentage (0-100)
	 */
	void setBacklightBrightness(uint8_t brightness);

private:
	DisplayManager() = default;
	~DisplayManager() = default;
	
	// Prevent copying and moving
	DisplayManager(const DisplayManager&) = delete;
	DisplayManager& operator=(const DisplayManager&) = delete;

	LGFX_driver m_tft;                      ///< LovyanGFX display driver instance
	static DisplayManager *s_instance;      ///< Singleton instance pointer

	// PWM backlight configuration constants
	static constexpr int BACKLIGHT_PIN = 3;           ///< GPIO pin for backlight control
	static constexpr int BACKLIGHT_CHANNEL = 0;       ///< LEDC channel number
	static constexpr int BACKLIGHT_FREQ = 5000;       ///< PWM frequency (5 kHz)
	static constexpr int BACKLIGHT_RESOLUTION = 8;    ///< PWM resolution (8-bit: 0-255)
};