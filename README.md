# Bike Pressure Monitor

A wireless tire pressure monitoring system (TPMS) for motorcycles built on ESP32-C3 with an LCD display powered by LVGL.

## Features

### Monitoring
- **Real-time monitoring** of front and rear tire pressure and temperature
- **BLE scanning** for TPMS sensors (compatible with standard BLE TPMS sensors)
- **Visual alerts** with color-coded pressure indicators (green/yellow/red)
- **Battery monitoring** for each sensor
- **Temperature-based UI** - bar colors change when temperature drops below 10°C
- **Auto-update** - UI refreshes automatically when new sensor data arrives

### Configuration
- **WiFi configuration mode** - web-based setup interface
- **Pairing mode** - guided on-screen sensor pairing process
- **Configurable ideal pressures** for front and rear tires
- **Brightness control** - 5 levels (10%, 30%, 50%, 75%, 100%)
- **Persistent settings** - all configuration stored in NVS (Non-Volatile Storage)

### Control
- **Touch-free operation** - single button control (GPIO9/BOOT button)
  - **Short press** (< 2s): cycle brightness levels
  - **Long press** (2-5s): enter sensor pairing mode
  - **Very long press** (> 5s): enter WiFi configuration mode
- **Automatic screen transitions** with splash screen on startup
- **Version display** - shows git version on splash screen

### Web Interface
- **WiFi AP Mode** - creates "TPMS-Config" access point (password: tpms1234)
- **Configuration portal** - accessible at http://192.168.4.1
- **Live sensor view** - real-time JSON API for sensor data
- **OTA updates** - over-the-air firmware updates via web interface
- **Factory reset** - clear all configuration and restart

## Hardware
- **Board:** ESP32-242412N (non-touch version)
- **MCU:** ESP32-C3 (single core, RISC-V architecture)
- **Flash:** 4MB
- **Display:** LCD with Lovyan GFX driver support
- **Sensors:** BLE TPMS sensors (front and rear wheels)
- **Button:** GPIO9 (BOOT button)

## Project Structure

```
.
├── main/
│   ├── main.cpp                 - Entry point (app_main)
│   ├── Application.cpp/h        - Main application logic and control flow
│   ├── UIController.cpp/h       - LVGL UI management
│   ├── DisplayManager.cpp/h     - LCD initialization (Lovyan GFX)
│   ├── State.cpp/h              - Global state management (singleton)
│   ├── ConfigManager.cpp/h      - NVS configuration handling
│   ├── PairController.cpp/h     - Sensor pairing logic
│   ├── WiFiManager.cpp/h        - WiFi AP mode management
│   ├── WebServer.cpp/h          - HTTP server for web interface
│   ├── TPMSScanCallbacks.cpp/h  - BLE scan callbacks
│   ├── TPMSUtil.cpp/h           - TPMS data parsing utilities
│   ├── LGFX_driver.h            - Lovyan GFX display configuration
│   ├── index_html.h             - Embedded HTML for web interface
│   └── UI/                      - SquareLine Studio generated UI
│       ├── ui.c/h               - Main UI code
│       ├── screens/             - Screen definitions
│       ├── components/          - UI components
│       ├── images/              - Image assets
│       └── fonts/               - Custom fonts
├── components/
│   ├── esp-nimble-cpp/          - NimBLE C++ wrapper for BLE
│   ├── lovyan-gfx/              - Display driver library
│   └── lvgl-custom/             - LVGL graphics library (v8)
├── squareline/                  - SquareLine Studio project files
├── CMakeLists.txt               - Main build configuration
└── sdkconfig                    - ESP-IDF configuration
```

## Architecture

The application follows a clean separation of concerns with a singleton-based architecture:

### Core Components

- **Application**: Singleton managing initialization, BLE setup, button logic, screen transitions, and mode switching (normal/pairing/config)
- **UIController**: Handles all LVGL UI updates and rendering via async callbacks
- **State**: Singleton storing global sensor data (pressure, temperature, battery, signal strength)
- **ConfigManager**: Persistent storage interface for NVS (addresses, pressures, brightness)
- **PairController**: State machine for guided sensor pairing process
- **WiFiManager**: Manages WiFi AP mode with event handlers
- **WebServer**: HTTP server with REST API and OTA update support
- **DisplayManager**: Initializes and configures the LCD display
- **TPMSScanCallbacks**: BLE advertisement parsing and sensor discovery

### Data Flow

1. BLE scan callbacks detect TPMS sensors and parse advertisements
2. Sensor data is stored in the global State singleton
3. Application triggers UI updates via LVGL async callbacks
4. UIController reads from State and updates LVGL widgets
5. Button presses are handled in Application control task
6. Configuration changes are persisted via ConfigManager

## Building

### Prerequisites

- **ESP-IDF**: v5.3.1
- **CMake**: 3.16 or higher
- **Python**: 3.8+ (for ESP-IDF tools)
- **Git**: For version tagging

### Build Commands

```bash
# Set target (first time only)
idf.py set-target esp32c3

# Configure (optional - opens menuconfig)
idf.py menuconfig

# Build
idf.py build

# Flash
idf.py -p COM_PORT flash

# Monitor serial output
idf.py -p COM_PORT monitor

# Flash and monitor in one command
idf.py -p COM_PORT flash monitor
```

### Build with specific IDF version

```powershell
# PowerShell
$env:IDF_PATH = 'C:/Espressif/frameworks/esp-idf-v5.3.1/'
idf.py build
```

## Configuration

### Default Settings (NVS)

Configuration is stored in Non-Volatile Storage and persists across reboots:

```json
{
  "front_address": "80:ea:ca:10:05:32",
  "rear_address": "81:ea:ca:20:04:10",
  "front_ideal_psi": 36.0,
  "rear_ideal_psi": 42.0,
  "brightness_index": 4
}
```

### WiFi Configuration Mode

Access the web interface to configure settings:

1. **Long press BOOT button** (> 5 seconds) to enter WiFi config mode
2. **Connect** to WiFi network: `TPMS-Config` (password: `tpms1234`)
3. **Open browser** to: `http://192.168.4.1`
4. Available endpoints:
   - `GET /` - Web interface
   - `GET /api/sensors` - Current sensor data (JSON)
   - `GET /api/config` - Current configuration (JSON)
   - `POST /api/config` - Update configuration
   - `POST /api/pair` - Manual sensor pairing
   - `POST /api/clear` - Factory reset
   - `POST /api/restart` - Restart device
   - `POST /api/ota` - Upload firmware for OTA update
   - `GET /api/ota/status` - OTA update status

### Pairing Mode

Guided sensor pairing with on-screen instructions:

1. **Long press BOOT button** (2-5 seconds) to enter pairing mode
2. **Scan** for front sensor (60 second timeout)
3. **Press button** to confirm when sensor is detected
4. **Scan** for rear sensor (60 second timeout)
5. **Press button** to confirm when sensor is detected
6. Configuration is saved and device restarts automatically

## Operation Modes

The device operates in three distinct modes:

### 1. Normal Mode (Default)
- Displays real-time tire pressure and temperature
- Monitors both front and rear TPMS sensors
- Short button press: cycle brightness
- Long press: enter pairing mode
- Very long press: enter WiFi config mode

### 2. Pairing Mode
- Guides user through sensor discovery
- Shows scanning status and found sensors
- Button confirms selection and advances to next step
- Auto-saves configuration on completion

### 3. WiFi Configuration Mode
- Starts WiFi AP: TPMS-Config
- Runs HTTP server on 192.168.4.1
- Provides web interface for all settings
- Supports OTA firmware updates
- Long press button again to exit

## UI Screens

### Splash Screen
- Displays app name and version (from git tag)
- Shows for 3 seconds on startup
- Version format: `v1.0.0` or git describe output

### Main Screen
- **Front tire**: Pressure (PSI), temperature (°C), battery (%), signal strength
- **Rear tire**: Pressure (PSI), temperature (°C), battery (%), signal strength
- **Color coding**:
  - Green: Pressure within ±3 PSI of ideal
  - Yellow: Pressure ±3-5 PSI from ideal
  - Red: Pressure > 5 PSI from ideal or < 10°C temperature
- **Status indicators**: Last update time, connection status

### Pairing Screen
- Current pairing step (front/rear)
- Scanning animation
- List of discovered sensors
- Confirmation prompts
- Timeout warnings

## Binary Size Optimizations

The project is optimized to fit within the ESP32-C3's flash constraints:

- **Compiler optimization**: `-Os` (size optimization)
- **Log levels**: 
  - Application: WARN
  - Bootloader: WARN
  - BLE/NimBLE: WARN
- **BLE configuration**:
  - Reduced connection limits
  - Optimized buffer sizes
- **LVGL configuration**:
  - Custom configuration via `lv_conf.h`
  - Limited widget set
  - Optimized rendering

## Development

### UI Design
UI is designed using **SquareLine Studio** (project files in `squareline/`):
- Export to `main/UI/`
- Includes screens, components, images, and fonts
- LVGL v8 compatible
- Custom theme with pressure-aware colors

### Version Management
- Version is automatically extracted from git tags during build
- Format: `git describe --tags --always --dirty`
- Displayed on splash screen
- Fallback: `1.0.0-dev` if git is unavailable

### Adding New Sensors
1. Update `TPMSUtil.cpp` to parse new sensor format
2. Add sensor type detection in `TPMSScanCallbacks.cpp`
3. Test pairing and data parsing
4. Update web API if needed

## Troubleshooting

### Sensors Not Detected
- Ensure tires are moving or recently moved (sensors may sleep)
- Check battery level in paired sensors
- Verify BLE is not blocked by other devices
- Try re-pairing using pairing mode

### Display Issues
- Check `LGFX_driver.h` for correct pin configuration
- Verify SPI connection and power supply
- Adjust brightness if screen is too dark

### WiFi Configuration Not Accessible
- Ensure you held button for > 5 seconds
- Look for "TPMS-Config" WiFi network
- Try connecting with password: `tpms1234`
- Check device is not in pairing mode instead

### Build Errors
- Ensure ESP-IDF v5.3.1 is installed
- Run `idf.py clean` before rebuilding
- Check submodules are initialized: `git submodule update --init --recursive`

## License

This project is provided as-is for educational and personal use.

## Repository

**GitHub**: [screemerpl/bike_pressure_monitor](https://github.com/screemerpl/bike_pressure_monitor)  
**Branch**: develop

## Author

Created for motorcycle tire pressure monitoring with ESP32-C3.

---

**Last Updated**: November 2025
