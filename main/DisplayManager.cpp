#include "DisplayManager.h"
#include "UI/ui.h"
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_log.h>

// Display resolution configuration
#define TFT_HOR_RES 240
#define TFT_VER_RES 240
#define BYTES_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))
#define TFT_ROTATION LV_DISPLAY_ROTATION_0

// LVGL draw buffer size - allocated for 1/10th of the screen
#define DRAW_BUF_SIZE                                                          \
	(TFT_HOR_RES * TFT_VER_RES * BYTES_PER_PIXEL) /                            \
		10 // buffer for 1/5 of the screen

// LVGL draw buffers (double buffering for smoother rendering)
static unsigned char *lv_draw_buf_mem = nullptr;
static unsigned char *lv_draw_buf_mem2 = nullptr;

static const char *TAG = "DisplayManager";

// Singleton instance pointer
DisplayManager *DisplayManager::s_instance = nullptr;

/**
 * @brief LVGL display flush callback function
 * @details Called by LVGL when it needs to render graphics to the display
 * @param disp LVGL display object
 * @param area Area of the screen to update
 * @param px_map Pixel data buffer
 */
extern "C" void my_disp_flush(lv_display_t *disp, const lv_area_t *area,
							  uint8_t *px_map) {
	if (DisplayManager::instance()) {
		DisplayManager::instance()->flushScreen(disp, area, px_map);
	} else {
		lv_display_flush_ready(disp);
	}
}

/**
 * @brief Flush screen buffer to display via DMA
 * @details Swaps byte order and pushes pixel data to the display using DMA
 * @param disp LVGL display object
 * @param area Screen area to update
 * @param px_map Pixel data buffer
 */
void DisplayManager::flushScreen(lv_display_t *disp, const lv_area_t *area,
								 uint8_t *px_map) {
	// End any ongoing write operation
	if (m_tft.getStartCount() == 0) {
		m_tft.endWrite();
	}

	// Calculate area dimensions
	const uint32_t w = lv_area_get_width(area);
	const uint32_t h = lv_area_get_height(area);

	// Prepare pixel buffer
	uint16_t *src16 = reinterpret_cast<uint16_t *>(px_map);
	const size_t pixels = (size_t)w * (size_t)h;

	// Swap bytes for correct color display (RGB565 byte order)
	for (size_t i = 0; i < pixels; ++i) {
		src16[i] = lv_swap_bytes_16(src16[i]);
	}
	
	// Push image data to display via DMA
	m_tft.pushImageDMA(area->x1, area->y1, w, h, src16);

	// Notify LVGL that flushing is complete
	lv_disp_flush_ready(disp);
}

/**
 * @brief Initialize display hardware and LVGL library
 * @details Sets up PWM backlight control, initializes TFT display driver,
 *          allocates LVGL draw buffers, and initializes UI
 */
void DisplayManager::init() {
    // Set singleton instance pointer
    DisplayManager::s_instance = this;
	
	// Configure LEDC timer for PWM backlight control
	ledc_timer_config_t ledc_timer = {
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.duty_resolution = static_cast<ledc_timer_bit_t>(BACKLIGHT_RESOLUTION),
		.timer_num = LEDC_TIMER_0,
		.freq_hz = BACKLIGHT_FREQ,
		.clk_cfg = LEDC_AUTO_CLK
	};
	ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
	
	// Configure LEDC channel for backlight pin
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
	
	// Set initial brightness to maximum (100%)
	setBacklightBrightness(100);
	
    // Initialize TFT display driver
    if (!m_tft.init()) {
        ESP_LOGE(TAG, "m_tft.init() failed");
        return;
    }
    
    // Verify panel is properly configured
    if (m_tft.panel() == nullptr) {
        ESP_LOGW(TAG, "m_tft.panel() == nullptr - verify LovyanGFX board/panel configuration");
        return;
    }

    // Initialize DMA for faster display updates
    if (m_tft.panel() != nullptr) {
        m_tft.initDMA();
    } else {
        ESP_LOGW(TAG, "Skipping initDMA because panel is null");
    }

    // Start write transaction and clear screen
    m_tft.startWrite();
    m_tft.setColor(0, 0, 0);

    // Initialize LVGL library
    lv_init();

	// Allocate first LVGL draw buffer (DMA capable memory)
	lv_draw_buf_mem = (unsigned char *)heap_caps_malloc(
		DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
	if (!lv_draw_buf_mem) {
		ESP_LOGE(TAG, "Failed to allocate LVGL draw buffer");
		return;
	}

	// Allocate second LVGL draw buffer for double buffering
	lv_draw_buf_mem2 = (unsigned char *)heap_caps_malloc(
		DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
	if (!lv_draw_buf_mem2) {
		ESP_LOGE(TAG, "Failed to allocate LVGL draw buffer");
		return;
	}

	// Create LVGL display object and configure buffers
	lv_display_t *disp;
	disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
	lv_display_set_flush_cb(disp, my_disp_flush);
	lv_display_set_buffers(disp, lv_draw_buf_mem, lv_draw_buf_mem2,
						   DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

	// Initialize SquareLine Studio generated UI
	ui_init();

	ESP_LOGI(TAG, "Display setup done");
}

/**
 * @brief Set display backlight brightness via PWM
 * @param brightness Brightness level in percentage (0-100%)
 */
void DisplayManager::setBacklightBrightness(uint8_t brightness) {
	// Clamp brightness to valid range (0-100%)
	if (brightness > 100) {
		brightness = 100;
	}
	
	// Convert percentage to duty cycle (0-255 for 8-bit resolution)
	uint32_t duty = (brightness * 255) / 100;
	
	// Update PWM duty cycle for backlight control
	ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, 
	                               static_cast<ledc_channel_t>(BACKLIGHT_CHANNEL), 
	                               duty));
	ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, 
	                                  static_cast<ledc_channel_t>(BACKLIGHT_CHANNEL)));
	
	ESP_LOGI(TAG, "Backlight brightness set to %d%% (duty: %lu/255)", brightness, duty);
}