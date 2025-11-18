#include "DisplayManager.h"
#include "UI/ui.h"
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_log.h>

#define TFT_HOR_RES 240
#define TFT_VER_RES 240
#define BYTES_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))
#define TFT_ROTATION LV_DISPLAY_ROTATION_0

#define DRAW_BUF_SIZE                                                          \
	(TFT_HOR_RES * TFT_VER_RES * BYTES_PER_PIXEL) /                            \
		10 // buffer for 1/5 of the screen
static unsigned char *lv_draw_buf_mem = nullptr;
static unsigned char *lv_draw_buf_mem2 = nullptr;

static const char *TAG = "DisplayManager";

DisplayManager *DisplayManager::s_instance = nullptr;

extern "C" void my_disp_flush(lv_display_t *disp, const lv_area_t *area,
							  uint8_t *px_map) {
	if (DisplayManager::instance()) {
		DisplayManager::instance()->flushScreen(disp, area, px_map);
	} else {
		lv_display_flush_ready(disp);
	}
}
#define SWAP_BYTES(x) ((((x) & 0xFF) << 8) | (((x) >> 8) & 0xFF))
void DisplayManager::flushScreen(lv_display_t *disp, const lv_area_t *area,
								 uint8_t *px_map) {
	if (tft.getStartCount() == 0) {
		tft.endWrite();
	}

	const uint32_t w = lv_area_get_width(area);
	const uint32_t h = lv_area_get_height(area);

	uint16_t *src16 = reinterpret_cast<uint16_t *>(px_map);
	const size_t pixels = (size_t)w * (size_t)h;

	for (size_t i = 0; i < pixels; ++i) {
		src16[i] = lv_swap_bytes_16(src16[i]);
		//       src16[i] = SWAP_BYTES(src16[i]);
	}
	tft.pushImageDMA(area->x1, area->y1, w, h, src16);

	lv_disp_flush_ready(disp); /* tell lvgl that flushing is done */
}

void DisplayManager::init() {
    DisplayManager::s_instance = this;
	
	// Configure LEDC for PWM backlight control
	ledc_timer_config_t ledc_timer = {
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.duty_resolution = static_cast<ledc_timer_bit_t>(BACKLIGHT_RESOLUTION),
		.timer_num = LEDC_TIMER_0,
		.freq_hz = BACKLIGHT_FREQ,
		.clk_cfg = LEDC_AUTO_CLK
	};
	ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
	
	ledc_channel_config_t ledc_channel = {
		.gpio_num = BACKLIGHT_PIN,
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.channel = static_cast<ledc_channel_t>(BACKLIGHT_CHANNEL),
		.intr_type = LEDC_INTR_DISABLE,
		.timer_sel = LEDC_TIMER_0,
		.duty = 0,
		.hpoint = 0,
		.flags = {}
	};
	ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
	
	// Set initial brightness to 100%
	setBacklightBrightness(100);
	
    // bezpieczne wywołania inicjalizujące
    if (!tft.init()) {
        ESP_LOGE(TAG, "tft.init() failed");
        return;
    }
    // sprawdź panel przed wywołaniem init/initDMA
    // (jeśli panel nie został skonfigurowany, init() w Lovyan może
    // dereferencjonować nullptr)
    if (tft.panel() == nullptr) {
        ESP_LOGW(TAG, "tft.panel() == nullptr - verify LovyanGFX board/panel "
					  "configuration");
        // możesz tu ustawić panel ręcznie jeśli masz Panel_Device* lub zwrócić
    }

    // sprawdzaj przed initDMA
    if (tft.panel() != nullptr) {
        tft.initDMA();
    } else {
        ESP_LOGW(TAG, "Skipping initDMA because panel is null");
    }

    tft.startWrite();
    tft.setColor(0, 0, 0);

    lv_init();
	//lv_tick_set_cb(xTaskGetTickCount);

	const uint32_t buf_lines =
		40; // ile linii bufor ma przechowywać - dostosuj jeśli trzeba
	lv_draw_buf_mem = (unsigned char *)heap_caps_malloc(
		DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
	if (!lv_draw_buf_mem) {
		ESP_LOGE(TAG, "Failed to allocate LVGL draw buffer");
		return;
	}

	lv_draw_buf_mem2 = (unsigned char *)heap_caps_malloc(
		DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
	if (!lv_draw_buf_mem2) {
		ESP_LOGE(TAG, "Failed to allocate LVGL draw buffer");
		return;
	}

	lv_display_t *disp;
	disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
	lv_display_set_flush_cb(disp, my_disp_flush);
	lv_display_set_buffers(disp, lv_draw_buf_mem, lv_draw_buf_mem2,
						   DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);


	ui_init();

	printf("Setup done\n");
}

void DisplayManager::setBacklightBrightness(uint8_t brightness) {
	// Clamp brightness to 0-100%
	if (brightness > 100) {
		brightness = 100;
	}
	
	// Convert percentage to duty cycle (0-255 for 8-bit resolution)
	uint32_t duty = (brightness * 255) / 100;
	
	ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, 
	                               static_cast<ledc_channel_t>(BACKLIGHT_CHANNEL), 
	                               duty));
	ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, 
	                                  static_cast<ledc_channel_t>(BACKLIGHT_CHANNEL)));
	
	ESP_LOGI(TAG, "Backlight brightness set to %d%% (duty: %lu/255)", brightness, duty);
}