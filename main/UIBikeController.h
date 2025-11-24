/**
 * @file UIBikeController.h
 * @brief UI controller for bike main screen
 * @details Handles updating LVGL UI elements for TPMS sensors on the main bike screen.
 *          Manages sensor displays, alerts, and blinking effects.
 */

#pragma once

#include "TPMSSensor.h"
#include <cstdint>

/**
 * @class UIBikeController
 * @brief Controller for main bike screen UI elements
 * @details Responsibilities:
 *          - Initialize labels on the main (bike) screen and manage UI widgets
 *          - Update sensor displays (pressure, temperature, battery)
 *          - Update pressure status icons and BLE connection indicators
 *          - Handle alert icon blinking (250ms) and label blinking (500ms)
 *          - Apply color coding based on pressure thresholds and temperature
 *
 * This class was introduced to isolate the main-screen UI logic from the LVGL
 * lifecycle and screen transition responsibilities (which remain in `UIController`).
 */
class UIBikeController {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to UIBikeController singleton
     */
    static UIBikeController &instance();

    /**
     * @brief Initialize all labels with default values
     * @details Sets pressure unit label and clears all sensor displays to "---"
     */
    void initializeLabels();

    /**
     * @brief Update all sensor UI elements
     * @param frontSensor Front tire sensor (nullptr if not available)
     * @param rearSensor Rear tire sensor (nullptr if not available)
     * @param frontIdealPSI Target pressure for front tire
     * @param rearIdealPSI Target pressure for rear tire
     * @param currentTime Current timestamp in milliseconds
     * @details Updates pressure/temp/battery displays and applies alert blinking.
     *          Missing sensors trigger label blinking (500ms period).
     */
    void updateSensorUI(TPMSSensor *frontSensor, TPMSSensor *rearSensor,
                        float frontIdealPSI, float rearIdealPSI,
                        uint32_t currentTime);

    /**
     * @brief Get current alert blink state
     * @return true if alert icons should be visible
     */
    bool getAlertBlinkState() const { return m_alertBlinkState; }

    /**
     * @brief Update alert and label blink states
     * @param currentTime Current timestamp in milliseconds
     * @details Toggles alert blink every 250ms, label blink every 500ms
     */
    void updateAlertBlinkState(uint32_t currentTime);
    
    /**
     * @brief Clear front sensor display
     * @param applyBlink If true, apply 500ms label blink effect
     */
    void clearFrontSensorUI(bool applyBlink = false);

    /**
     * @brief Clear rear sensor display
     * @param applyBlink If true, apply 500ms label blink effect
     */
    void clearRearSensorUI(bool applyBlink = false);

private:
    UIBikeController() = default;
    ~UIBikeController() = default;

    // Disable copy/move
    UIBikeController(const UIBikeController &) = delete;
    UIBikeController &operator=(const UIBikeController &) = delete;

    /**
     * @brief Update front sensor display
     * @param frontSensor Front tire sensor data
     * @param frontIdealPSI Target pressure for front tire
     * @param currentTime Current timestamp in milliseconds
     * @details Updates pressure (PSI/BAR), temperature, battery level,
     *          pressure icon (green/yellow/red), BLE status icon,
     *          and temperature bar color (blue if <10°C)
     */
    void updateFrontSensorUI(TPMSSensor *frontSensor, float frontIdealPSI,
                             uint32_t currentTime);

    /**
     * @brief Update rear sensor display
     * @param rearSensor Rear tire sensor data
     * @param rearIdealPSI Target pressure for rear tire
     * @param currentTime Current timestamp in milliseconds
     * @details Same as updateFrontSensorUI but for rear tire UI elements
     */
    void updateRearSensorUI(TPMSSensor *rearSensor, float rearIdealPSI,
                            uint32_t currentTime);



    /**
     * @brief Update alert icons based on sensor alert flags
     * @param alertFront Front sensor alert status
     * @param alertRear Rear sensor alert status
     * @details Blinks alert icons (250ms period) if any sensor has alert flag
     */
    void updateAlertIcons(bool alertFront, bool alertRear);

    bool m_alertBlinkState = false;      ///< Alert icon blink state (250ms period)
    uint32_t m_lastBlinkTime = 0;        ///< Last alert blink toggle timestamp

    bool m_labelBlinkState = false;      ///< Label blink state (500ms period)
    uint32_t m_lastLabelBlinkTime = 0;   ///< Last label blink toggle timestamp
};