# Motorcycle TPMS Monitor

A wireless tire pressure monitoring system (TPMS) for motorcycles built on ESP32-C3 with an LCD display powered by LVGL.

## Features

- **Real-time monitoring** of front and rear tire pressure and temperature
- **BLE scanning** for TPMS sensors (compatible with standard BLE TPMS sensors)
- **Visual alerts** with color-coded pressure indicators (green/yellow/red)
- **Battery monitoring** for each sensor
- **Temperature-based UI** - bar colors change when temperature drops below 10°C
- **Configurable ideal pressures** via JSON configuration
- **Brightness control** - 5 levels (10%, 30%, 50%, 75%, 100%)
- **Persistent settings** - configuration stored in NVS
- **Touch-free operation** - single button control (GPIO9)
  - Short press: cycle brightness
  - Long press (2s): enter pairing mode

## Hardware
- **BOARD:** ESP32-242412N (non touch)
- **MCU:** ESP32-C3 (single core, RISC-V)
- **Display:** LCD with Lovyan GFX driver support
- **Sensors:** BLE TPMS sensors (front and rear)
- **Button:** GPIO9 (BOOT button)

## Project Structure

```
.
├── main/
│   ├── Application.cpp/h        - Main application logic
│   ├── UIController.cpp/h       - LVGL UI management
│   ├── ConfigManager.cpp/h      - NVS configuration handling
│   ├── DisplayManager.cpp/h     - LCD initialization
│   ├── State.cpp/h              - Global state management
│   ├── TPMSScanCallbacks.cpp/h  - BLE scan callbacks
│   ├── TPMSUtil.cpp/h           - TPMS data parsing
│   └── UI/                      - SquareLine Studio generated UI
├── components/
│   ├── esp-nimble-cpp/          - NimBLE C++ wrapper
│   ├── lovyan-gfx/              - Display driver
│   └── lvgl-custom/             - LVGL graphics library
└── CMakeLists.txt
```

## Architecture

The application follows a clean separation of concerns:

- **Application**: Handles initialization, BLE setup, button logic, and screen transitions
- **UIController**: Manages all LVGL UI updates and rendering
- **State**: Singleton for global sensor data management
- **ConfigManager**: Persistent storage of settings (addresses, pressures, brightness)

## Building

### Prerequisites

- ESP-IDF v5.3.1
- CMake 3.16+

### Build Commands

```bash
# Configure
idf.py set-target esp32c3

# Build
idf.py build

# Flash
idf.py -p PORT flash

# Monitor
idf.py -p PORT monitor
```

## Configuration

Default configuration stored in NVS:

```json
{
  "front_address": "80:ea:ca:10:05:32",
  "rear_address": "81:ea:ca:20:04:10",
  "front_ideal_psi": 36.0,
  "rear_ideal_psi": 42.0,
  "brightness_index": 4
}
```

## Binary Size Optimizations

The project uses compiler size optimization (`-Os`) and reduced logging to fit within flash constraints:

- Compiler optimization: SIZE mode
- Log levels: WARN (application, bootloader, BLE)
- Reduced BLE connection limits

## License

This project is provided as-is for educational and personal use.

## Author

Created for motorcycle tire pressure monitoring with ESP32-C3.
