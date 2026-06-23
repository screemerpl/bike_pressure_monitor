/**
 * @file UICarController.h
 * @brief UI controller for car main screen
 * @details Handles updating LVGL UI elements for TPMS sensors on the main car screen.
 *          Manages sensor displays, alerts, and blinking effects for 4 sensors.
 */

#pragma once

#include "TPMSSensor.h"
#include <cstdint>

/**
 * @class UICarController
 * @brief Controller for main car screen UI elements (4 sensors)
 * @details Responsibilities:
 *          - Initialize labels on car main screen
 *          - Update 4 sensor displays (pressure, temperature, battery)
 *          - Handle alert icon blinking for 4 sensors
 *          - Handle label blinking for unsynchronized sensors
 *          - Apply color coding based on pressure thresholds
 */
class UICarController {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to UICarController singleton
     */
    static UICarController &instance();

    /**
     * @brief Initialize all labels with default values
     * @details Sets pressure unit label and clears all 4 sensor displays to "---"
     */
    void initializeLabels();

    /**
     * @brief Update all sensor UI elements (4 sensors)
     * @param s1 Sensor for front-left (nullptr if not available)
     * @param s2 Sensor for front-right (nullptr if not available)
     * @param s3 Sensor for rear-left (nullptr if not available)
     * @param s4 Sensor for rear-right (nullptr if not available)
     * @param ideal1 Ideal pressure for front-left
     * @param ideal2 Ideal pressure for front-right
     * @param ideal3 Ideal pressure for rear-left
     * @param ideal4 Ideal pressure for rear-right
     * @param currentTime Current timestamp in milliseconds
     */
    void updateSensorUI(TPMSSensor *s1, TPMSSensor *s2, TPMSSensor *s3, TPMSSensor *s4,
                        float ideal1, float ideal2, float ideal3, float ideal4,
                        uint32_t currentTime,
                        const bool *hasLastReading = nullptr,
                        const float *lastPressurePSI = nullptr,
                        const bool *awaitingSync = nullptr);

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

private:
    UICarController() = default;
    ~UICarController() = default;

    // Disable copy/move
    UICarController(const UICarController &) = delete;
    UICarController &operator=(const UICarController &) = delete;

    void updateSensorUIInternal(int index, TPMSSensor *sensor, float idealPSI, uint32_t currentTime);
    void clearSensorUIInternal(int index, bool applyBlink,
                               bool hasLastReading = false,
                               float lastPressurePSI = 0.0f,
                               bool blinkBluetooth = false);
    void updateAlertIcons(bool a1, bool a2, bool a3, bool a4);

    bool m_alertBlinkState = false;      ///< Alert icon blink state (250ms period)
    uint32_t m_lastBlinkTime = 0;        ///< Last alert blink toggle timestamp

    bool m_labelBlinkState = false;      ///< Label blink state (500ms period)
    uint32_t m_lastLabelBlinkTime = 0;   ///< Last label blink toggle timestamp
};
