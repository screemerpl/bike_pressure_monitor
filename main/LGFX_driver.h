#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.h>
#include <lvgl.h>

#define off_pin 35
#define buf_size 15



class LGFX_driver : public lgfx::LGFX_Device {
	lgfx::Panel_GC9A01 _panel_instance;
	lgfx::Bus_SPI _bus_instance;

public:
	LGFX_driver(void) {
		
		{
			auto cfg = _bus_instance.config();

			// SPIバスの設定
			cfg.spi_host =
				SPI2_HOST; // 使用するSPIを選択  ESP32-S2,C3 : SPI2_HOST or
						   // SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
			// ※ ESP-IDFバージョンアップに伴い、VSPI_HOST ,
			// HSPI_HOSTの記述は非推奨になるため、エラーが出る場合は代わりにSPI2_HOST
			// , SPI3_HOSTを使用してください。
			cfg.spi_mode = 0; // SPI通信モードを設定 (0 ~ 3)
			cfg.freq_write =
				80000000; // 传输时的SPI时钟（最高80MHz，四舍五入为80MHz除以整数得到的值）
			cfg.freq_read = 20000000; // 接收时的SPI时钟
			cfg.spi_3wire = true;	  // 受信をMOSIピンで行う場合はtrueを設定
			cfg.use_lock = true;	  // 使用事务锁时设置为 true
			cfg.dma_channel =
				SPI_DMA_CH_AUTO; // 使用するDMAチャンネルを設定 (0=DMA不使用 /
								 // 1=1ch / 2=ch / SPI_DMA_CH_AUTO=自動設定)
			// ※
			// ESP-IDFバージョンアップに伴い、DMAチャンネルはSPI_DMA_CH_AUTO(自動設定)が推奨になりました。1ch,2chの指定は非推奨になります。
			cfg.pin_sclk = 6;  // SPIのSCLKピン番号を設定
			cfg.pin_mosi = 7;  // SPIのCLKピン番号を設定
			cfg.pin_miso = -1; // SPIのMISOピン番号を設定 (-1 = disable)
			cfg.pin_dc = 2;	   // SPIのD/Cピン番号を設定  (-1 = disable)

			_bus_instance.config(cfg); // 設定値をバスに反映します。
			_panel_instance.setBus(
				&_bus_instance); // バスをパネルにセットします。
		
			}
			{
		
			auto cfg0 = _panel_instance
						   .config(); // 表示パネル設定用の構造体を取得します。

			cfg0.pin_cs = 10;   // CSが接続されているピン番号   (-1 = disable)
			cfg0.pin_rst = -1;  // RSTが接続されているピン番号  (-1 = disable)
			cfg0.pin_busy = -1; // BUSYが接続されているピン番号 (-1 = disable)

			// ※ 以下の設定値はパネル毎に一般的な初期値が設定さ
			// BUSYが接続されているピン番号 (-1 =
			// disable)れていますので、不明な項目はコメントアウトして試してみてください。

			cfg0.memory_width = 240;	  // ドライバICがサポートしている最大の幅
			cfg0.memory_height = 240;  // ドライバICがサポートしている最大の高さ
			cfg0.panel_width = 240;	  // 実際に表示可能な幅
			cfg0.panel_height = 240;	  // 実際に表示可能な高さ
			cfg0.offset_x = 0;		  // パネルのX方向オフセット量
			cfg0.offset_y = 0;		  // パネルのY方向オフセット量
			cfg0.offset_rotation = 0;  // 值在旋转方向的偏移0~7（4~7是倒置的）
			cfg0.dummy_read_pixel = 8; // 在读取像素之前读取的虚拟位数
			cfg0.dummy_read_bits = 1;  // 读取像素以外的数据之前的虚拟读取位数
			cfg0.readable = false;	  // 如果可以读取数据，则设置为 true
			cfg0.invert = true;		  // 如果面板的明暗反转，则设置为 true
			cfg0.rgb_order = false; // 如果面板的红色和蓝色被交换，则设置为 true
			cfg0.dlen_16bit =
				false; // 对于以 16 位单位发送数据长度的面板，设置为 true
			cfg0.bus_shared = false; // 如果总线与 SD 卡共享，则设置为 true（使用
									// drawJpgFile 等执行总线控制）

			_panel_instance.config(cfg0);
			}

		setPanel(&_panel_instance); // 使用するパネルをセットします。
	}
};