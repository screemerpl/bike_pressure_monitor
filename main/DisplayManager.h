#pragma once

#include "LGFX_driver.h"

class DisplayManager {
public:
	void init();
	void flushScreen(lv_display_t *disp, const lv_area_t *area,
					 uint8_t *color_p);
	static DisplayManager *instance() {
		if (s_instance == nullptr) {
			s_instance = new DisplayManager();
		}
		return s_instance;
	}
	static void setBacklightBrightness(uint8_t brightness);

private:
	LGFX_driver tft;
	static DisplayManager *s_instance;

	// PWM configuration for backlight
	static constexpr int BACKLIGHT_PIN = 3;
	static constexpr int BACKLIGHT_CHANNEL = 0;
	static constexpr int BACKLIGHT_FREQ = 5000;	   // 5 kHz PWM frequency
	static constexpr int BACKLIGHT_RESOLUTION = 8; // 8-bit resolution (0-255)
};