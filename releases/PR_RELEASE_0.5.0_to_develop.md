# PR: Release v0.5.0 → develop

Title: Release v0.5.0 - merge release/0.5.0 into develop

## Summary
Merge release branch `release/0.5.0` into `develop`. This release includes the following highlights:

- Added car-mode UI (4 sensors) with `UICarController` and `ui_CarMain` screen.
- Extracted motorcycle UI logic into `UIBikeController` (2 sensors) and simplified `UIController` responsibilities.
- Updated `Application` to route UI updates to Bike/Car controllers based on `State::getMode()`.
- Updated README (renamed to Universal Pressure Monitor), created root `CHANGELOG.md` with release notes.
- Build / CMake rename to `universal_pressure_monitor` for artifact naming.

## Changelog
See `CHANGELOG.md` in the root of this PR for full details: v0.5.0

## Migration Notes
- If you are upgrading from prior releases:
  - Make sure to update NVS sensor keys if switching to car-mode (use the web UI to set `sensor_address_1..4` and `ideal_psi` values).
  - If you rely on older consumer-facing config keys, ensure you re-apply configuration.

## Testing
- Build: `idf.py build` successful and produced `build/universal_pressure_monitor.bin`.
- Recommend: flash on a test device and validate both motorcycle and car modes, pairing, web UI configuration, and sensor scanning.

## Approvals
- Tagging maintainers: @screemerpl

---

(You can copy this content into the GitHub PR description when creating the PR.)