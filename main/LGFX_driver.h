/**
 * @file LGFX_driver.h
 * @brief LovyanGFX display driver configuration
 * @details Configures GC9A01 240x240 round LCD display with SPI interface.
 *          Used by DisplayManager for low-level display initialization.
 *          
 *          Hardware configuration:
 *          - Display: GC9A01 round LCD (240x240 pixels)
 *          - Interface: SPI2_HOST
 *          - SPI Pins: SCLK=6, MOSI=7, DC=2, CS=10
 *          - SPI Speed: 80MHz (write), 20MHz (read)
 *          - DMA: Auto channel (SPI_DMA_CH_AUTO)
 *          - Display properties: Inverted colors, RGB order normal
 *          - LVGL buffer size: 15 (multiplier for display buffer allocation)
 *          
 *          Based on LovyanGFX library configuration system.
 */

#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.h>
#include <lvgl.h>

#define buf_size 15      ///< LVGL buffer size multiplier



/**
 * @class LGFX_driver
 * @brief LovyanGFX driver for GC9A01 round LCD
 * @details Configures SPI bus and panel settings for GC9A01 display.
 *          Inherits from lgfx::LGFX_Device and sets up:
 *          - SPI bus parameters (host, pins, speed, DMA)
 *          - Panel parameters (size, offset, rotation, color inversion)
 *          
 *          Used by DisplayManager::initDisplay() for hardware initialization.
 */
class LGFX_driver : public lgfx::LGFX_Device {
	lgfx::Panel_GC9A01 _panel_instance;  ///< GC9A01 panel driver instance
	lgfx::Bus_SPI _bus_instance;         ///< SPI bus driver instance

public:
	/**
	 * @brief Constructor - configures SPI bus and panel
	 * @details Initializes SPI2_HOST with:
	 *          - Write speed: 80MHz
	 *          - Read speed: 20MHz
	 *          - 3-wire SPI mode (MOSI used for receive)
	 *          - Auto DMA channel selection
	 *          - Pin mapping: SCLK=6, MOSI=7, DC=2, CS=10
	 *          
	 *          Panel configuration:
	 *          - Resolution: 240x240 (circular display)
	 *          - Color inversion: enabled
	 *          - Rotation offset: 0
	 *          - RGB order: normal (red/blue not swapped)
	 */
	LGFX_driver(void) {
		
		{
			auto cfg = _bus_instance.config();

			// SPI bus configuration
			cfg.spi_host = SPI2_HOST;     // ESP32-C3: SPI2_HOST or SPI3_HOST
			cfg.spi_mode = 0;             // SPI mode (0-3)
			cfg.freq_write = 80000000;    // Write SPI clock: 80MHz (max for display)
			cfg.freq_read = 20000000;     // Read SPI clock: 20MHz
			cfg.spi_3wire = true;         // 3-wire SPI: MOSI used for receive
			cfg.use_lock = true;          // Enable transaction lock
			cfg.dma_channel = SPI_DMA_CH_AUTO;  // Auto DMA channel selection
			cfg.pin_sclk = 6;             // SPI clock pin
			cfg.pin_mosi = 7;             // SPI MOSI pin
			cfg.pin_miso = -1;            // SPI MISO pin (disabled for 3-wire)
			cfg.pin_dc = 2;               // Data/Command pin

			_bus_instance.config(cfg);    // Apply bus configuration
			_panel_instance.setBus(&_bus_instance);  // Attach bus to panel
		
			}
			{
		
			auto cfg0 = _panel_instance.config();  // Get panel configuration structure

			cfg0.pin_cs = 10;      // Chip Select pin
			cfg0.pin_rst = -1;     // Reset pin (disabled)
			cfg0.pin_busy = -1;    // Busy pin (disabled)

			// Display dimensions and properties
			cfg0.memory_width = 240;      // Max width supported by IC
			cfg0.memory_height = 240;     // Max height supported by IC
			cfg0.panel_width = 240;       // Actual displayable width
			cfg0.panel_height = 240;      // Actual displayable height
			cfg0.offset_x = 0;            // X offset
			cfg0.offset_y = 0;            // Y offset
			cfg0.offset_rotation = 0;     // Rotation offset (0-7, 4-7 are inverted)
			cfg0.dummy_read_pixel = 8;    // Dummy bits before pixel read
			cfg0.dummy_read_bits = 1;     // Dummy bits before non-pixel read
			cfg0.readable = false;        // Data read capability
			cfg0.invert = true;           // Color inversion enabled
			cfg0.rgb_order = false;       // RGB order (false = normal, true = BGR)
			cfg0.dlen_16bit = false;      // 16-bit data length mode
			cfg0.bus_shared = false;      // Bus shared with SD card

			_panel_instance.config(cfg0);  // Apply panel configuration
			}

		setPanel(&_panel_instance);  // Attach configured panel to device
	}
};