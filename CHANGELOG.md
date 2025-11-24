# CHANGELOG

All notable changes to this project will be documented in this file.

## [v0.5.0] - 2025-11-24
### Added
- Car mode UI with 4 sensors: `UICarController` and `ui_CarMain` screen.
- `UIBikeController`: extracted motorcycle main screen logic out of `UIController`.
- `UIController` now only handles LVGL tick/timers and top-level screen transitions.
- Web configuration now supports `mode` (motorcycle/cars) and up to 4 sensor addresses for car mode.
- Release process: created `release/0.5.0` branch and annotated tag `v0.5.0`.

### Changed
- Project documentation updates: project name updated to `Universal Pressure Monitor` and README revamped to include car mode.
- `Application`: delegates sensor UI updates to appropriate controller (Bike/Car) based on `State::getMode()`.
- Refactor: Main-screen logic moved out from `UIController` into `UIBikeController` and `UICarController`.
- Build: `CMakeLists.txt` project name changed to `universal_pressure_monitor` for build artifact naming.

### Fixed
- Minor cleanup and comment clarifications across UI and application files.

### Notes / Migration
- If you upgrade from a previous motorcycle-only firmware, ensure you update NVS configuration for the `mode` and sensor addresses for car mode. Car sensor addresses are expected to be in keys `sensor_address_1..4` (Application and Web UI use different keys for car mode). When switching between modes, re-save the addresses via the web UI or API.

---

*This changelog was automatically generated and edited. Please review and expand details as necessary.*
