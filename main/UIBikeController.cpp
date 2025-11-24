/**
 * @file UIBikeController.cpp
 * @brief UI controller implementation for bike main screen
 * @details Implements LVGL UI updates for TPMS sensor display on main screen.
 *          Handles pressure thresholds, color coding, blinking effects,
 *          and unit conversions (PSI/BAR).
 */

#include "UIBikeController.h"
#include "State.h"
#include "UI/ui.h"
#include "UI/ui_themes.h"
#include "ui_theme_helper.h"
#include "lvgl.h"
#include "esp_log.h"
#include <cstdio>

/**
 * @brief Get singleton instance
 * @return Reference to UIBikeController singleton (static local variable)
 */
UIBikeController &UIBikeController::instance() {
    static UIBikeController controller;
    return controller;
}

/**
 * @brief Initialize all UI labels with default values
 * @details Sets pressure unit label from config and clears all
 *          sensor displays to "---", resets arcs/bars to 0, and
 *          sets icons to default (black TPMS, BT off, idle alert)
 */
void UIBikeController::initializeLabels() {
    State &state = State::getInstance();

    // Set unit label based on configuration
    lv_label_set_text(ui_Unit, state.getPressureUnit().c_str());

    lv_label_set_text(ui_Pressure1, "---");
    lv_label_set_text(ui_Pressure2, "---");
    lv_label_set_text(ui_TempText1, "-- °C");
    lv_label_set_text(ui_TempText2, "-- °C");
    lv_label_set_text(ui_BatteryText1, "--%");
    lv_label_set_text(ui_BatteryText2, "--%");
    lv_arc_set_value(ui_Battery2, 0);
    lv_arc_set_value(ui_Battery1, 0);
    lv_image_set_src(ui_TPMSicon1, &ui_img_tpmsblack_png);
    /* Chroma key removed - image sources set without colorkey */
}

/**
 * @brief Update all sensor UI elements
 * @param frontSensor Front tire sensor (nullptr if not synchronized)
 * @param rearSensor Rear tire sensor (nullptr if not synchronized)
 * @param frontIdealPSI Target pressure for front tire
 * @param rearIdealPSI Target pressure for rear tire
 * @param currentTime Current timestamp in milliseconds
 * @details Updates both front and rear sensor displays, applies blinking
 *          to unsynchronized sensors, and updates alert icons
 */
void UIBikeController::updateSensorUI(TPMSSensor *frontSensor, TPMSSensor *rearSensor,
                                      float frontIdealPSI, float rearIdealPSI,
                                      uint32_t currentTime) {
    bool alertFront = false;
    bool alertRear = false;

    if (frontSensor) {
        updateFrontSensorUI(frontSensor, frontIdealPSI, currentTime);

        alertFront = frontSensor->getAlert();
    } else {
        clearFrontSensorUI(true); // true = apply blink
    }

    if (rearSensor) {
        updateRearSensorUI(rearSensor, rearIdealPSI, currentTime);

        alertRear = rearSensor->getAlert();
    } else {
        clearRearSensorUI(true); // true = apply blink
    }

    updateAlertIcons(alertFront, alertRear);
}

/**
 * @brief Update front sensor UI display
 * @param frontSensor Front tire sensor data
 * @param frontIdealPSI Target pressure for front tire
 * @param currentTime Current timestamp in milliseconds
 * @details Updates pressure (PSI or BAR based on State::pressureUnit),
 *          temperature, battery level, pressure status icon:
 *          - Red: pressure < 75% of ideal
 *          - Yellow: pressure < 90% of ideal
 *          - Black: pressure >= 90% of ideal
 *          Temperature bar color: Blue if temp < 10°C, green otherwise
 *          BLE icon: ON if data received within last 200ms
 */
void UIBikeController::updateFrontSensorUI(TPMSSensor *frontSensor,
                                           float frontIdealPSI,
                                           uint32_t currentTime) {
    char buf[16];
    State &state = State::getInstance();

    // Display pressure in selected unit
    if (state.getPressureUnit() == "BAR") {
        snprintf(buf, sizeof(buf), "%.2f", frontSensor->getPressureBar());
    } else {
        snprintf(buf, sizeof(buf), "%.1f", frontSensor->getPressurePSI());
    }
    lv_label_set_text(ui_Pressure1, buf);

    // Reset label color to theme text color when sensor is synchronized
    lv_obj_set_style_text_color(ui_Pressure1,
                                ui_theme_get_lv_color(UI_THEME_COLOR_TEXT),
                                LV_PART_MAIN );
    snprintf(buf, sizeof(buf), "%.1f °C", frontSensor->getTemperatureC());
    lv_label_set_text(ui_TempText1, buf);

    snprintf(buf, sizeof(buf), "%d%%", frontSensor->getBatteryLevel());
    lv_label_set_text(ui_BatteryText1, buf);

    lv_arc_set_value(ui_Battery1, static_cast<int>(frontSensor->getBatteryLevel()));
    lv_bar_set_value(ui_BatteryBar1, static_cast<int>(frontSensor->getTemperatureC()),
                     LV_ANIM_ON);

    // Change bar color to blue if temperature is below 10°C
    if (frontSensor->getTemperatureC() < 10.0f) {
        // On cold temperatures use STANDARD and BRIGHT theme colors
        lv_obj_set_style_bg_color(ui_BatteryBar1,
                      ui_theme_get_lv_color(UI_THEME_COLOR_STANDARD),
                      LV_PART_MAIN);
        lv_obj_set_style_bg_color(ui_BatteryBar1,
                      ui_theme_get_lv_color(UI_THEME_COLOR_HOT),
                      LV_PART_INDICATOR);
    } else {
        // Default (warmer) temperature: also use STANDARD and BRIGHT themed colors
        lv_obj_set_style_bg_color(ui_BatteryBar1,
                      ui_theme_get_lv_color(UI_THEME_COLOR_STANDARD),
                      LV_PART_MAIN);
        lv_obj_set_style_bg_color(ui_BatteryBar1,
                      ui_theme_get_lv_color(UI_THEME_COLOR_BRIGHT),
                      LV_PART_INDICATOR);
    }

    // Update pressure indicator icon
    if (frontSensor->getPressurePSI() < frontIdealPSI * 0.75f) {
        lv_image_set_src(ui_TPMSicon1, &ui_img_tpmsred_png);
    } else if (frontSensor->getPressurePSI() < frontIdealPSI * 0.9f) {
        lv_image_set_src(ui_TPMSicon1, &ui_img_tpmsyellow_png);
    } else {
        lv_image_set_src(ui_TPMSicon1, &ui_img_tpmsblack_png);

    }

    // Update BLE connection status icon
    if (frontSensor->getTimestamp() + 200 < currentTime) {
        lv_image_set_src(ui_BTicon1, &ui_img_btoff_png);

    } else {
        lv_image_set_src(ui_BTicon1, &ui_img_bton_png);
    }
}

/**
 * @brief Update rear sensor UI display
 * @param rearSensor Rear tire sensor data
 * @param rearIdealPSI Target pressure for rear tire
 * @param currentTime Current timestamp in milliseconds
 * @details Same logic as updateFrontSensorUI but for rear tire UI elements
 *          (ui_Pressure2, ui_TempText2, ui_BatteryText2, ui_Battery2, ui_BatteryBar2, ui_TPMSicon2, ui_BTicon2)
 */
void UIBikeController::updateRearSensorUI(TPMSSensor *rearSensor, float rearIdealPSI,
                                          uint32_t currentTime) {
    char buf[16];
    State &state = State::getInstance();

    // Display pressure in selected unit
    if (state.getPressureUnit() == "BAR") {
        snprintf(buf, sizeof(buf), "%.2f", rearSensor->getPressureBar());
    } else {
        snprintf(buf, sizeof(buf), "%.1f", rearSensor->getPressurePSI());
    }
    lv_label_set_text(ui_Pressure2, buf);

    // Reset label color to theme text color when sensor is synchronized
    lv_obj_set_style_text_color(ui_Pressure2,
                                ui_theme_get_lv_color(UI_THEME_COLOR_TEXT),
                                LV_PART_MAIN );
    snprintf(buf, sizeof(buf), "%.1f °C", rearSensor->getTemperatureC());
    lv_label_set_text(ui_TempText2, buf);

    snprintf(buf, sizeof(buf), "%d%%", rearSensor->getBatteryLevel());
    lv_label_set_text(ui_BatteryText2, buf);

    lv_arc_set_value(ui_Battery2, static_cast<int>(rearSensor->getBatteryLevel()));
    lv_bar_set_value(ui_BatteryBar2, static_cast<int>(rearSensor->getTemperatureC()),
                     LV_ANIM_ON);

    // Change bar color to blue if temperature is below 10°C
    if (rearSensor->getTemperatureC() < 10.0f) {
        // On cold temperatures use STANDARD and BRIGHT theme colors
        lv_obj_set_style_bg_color(ui_BatteryBar2,
                      ui_theme_get_lv_color(UI_THEME_COLOR_STANDARD),
                      LV_PART_MAIN);
        lv_obj_set_style_bg_color(ui_BatteryBar2,
                      ui_theme_get_lv_color(UI_THEME_COLOR_HOT),
                      LV_PART_INDICATOR);
    } else {
        // Default (warmer) temperature: also use STANDARD and BRIGHT themed colors
        lv_obj_set_style_bg_color(ui_BatteryBar2,
                      ui_theme_get_lv_color(UI_THEME_COLOR_STANDARD),
                      LV_PART_MAIN);
        lv_obj_set_style_bg_color(ui_BatteryBar2,
                      ui_theme_get_lv_color(UI_THEME_COLOR_BRIGHT),
                      LV_PART_INDICATOR);
    }

    // Update pressure indicator icon
        if (rearSensor->getPressurePSI() < rearIdealPSI * 0.75f) {
        lv_image_set_src(ui_TPMSicon2, &ui_img_tpmsred_png);
    } else if (rearSensor->getPressurePSI() < rearIdealPSI * 0.9f) {
        lv_image_set_src(ui_TPMSicon2, &ui_img_tpmsyellow_png);
    } else {
        lv_image_set_src(ui_TPMSicon2, &ui_img_tpmsblack_png);
    }

    // Update BLE connection status icon
    if (rearSensor->getTimestamp() + 200 < currentTime) {
        lv_image_set_src(ui_BTicon2, &ui_img_btoff_png);

    } else {
        lv_image_set_src(ui_BTicon2, &ui_img_bton_png);

    }
}

/**
 * @brief Clear front sensor UI when sensor is not available
 * @param applyBlink If true, apply 500ms blink effect to pressure label
 * @details Resets all front sensor UI elements to default/empty state.
 *          Blinking (white <-> black) indicates sensor is not synchronized.
 */
void UIBikeController::clearFrontSensorUI(bool applyBlink) {
    lv_label_set_text(ui_Pressure1, "---");

    // Apply blinking effect only if requested: white when blink state is true,
    // black when false
    if (applyBlink) {
        if (m_labelBlinkState) {
            // Blink: use theme text color
            lv_obj_set_style_text_color(ui_Pressure1,
                                        ui_theme_get_lv_color(UI_THEME_COLOR_TEXT),
                                        LV_PART_MAIN);
        } else {
            // Blink off: use background color from theme
            lv_obj_set_style_text_color(ui_Pressure1,
                                        ui_theme_get_lv_color(UI_THEME_COLOR_BACKGROUND),
                                        LV_PART_MAIN);
        }
    } else {
        // Reset to theme text color when not blinking
        lv_obj_set_style_text_color(ui_Pressure1,
                        ui_theme_get_lv_color(UI_THEME_COLOR_TEXT),
                        LV_PART_MAIN);
    }

    lv_label_set_text(ui_TempText1, "-- °C");
    lv_label_set_text(ui_BatteryText1, "--%");
    lv_arc_set_value(ui_Battery1, 0);
    lv_bar_set_value(ui_BatteryBar1, -10, LV_ANIM_ON);
    lv_image_set_src(ui_TPMSicon1, &ui_img_tpmsblack_png);
    lv_image_set_src(ui_BTicon1, &ui_img_btoff_png);
    /* Chroma key removed - image sources set without colorkey */
}

/**
 * @brief Clear rear sensor UI when sensor is not available
 * @param applyBlink If true, apply 500ms blink effect to pressure label
 * @details Same as clearFrontSensorUI but for rear tire UI elements
 */
void UIBikeController::clearRearSensorUI(bool applyBlink) {
    lv_label_set_text(ui_Pressure2, "---");

    // Apply blinking effect only if requested: white when blink state is true,
    // black when false
    if (applyBlink) {
        if (m_labelBlinkState) {
            // Blink on: theme text color
            lv_obj_set_style_text_color(ui_Pressure2,
                                        ui_theme_get_lv_color(UI_THEME_COLOR_TEXT),
                                        LV_PART_MAIN);
        } else {
            // Blink off: theme background color
            lv_obj_set_style_text_color(ui_Pressure2,
                                        ui_theme_get_lv_color(UI_THEME_COLOR_BACKGROUND),
                                        LV_PART_MAIN);
        }
    } else {
        // Reset to theme text color when not blinking
        lv_obj_set_style_text_color(ui_Pressure2,
                        ui_theme_get_lv_color(UI_THEME_COLOR_TEXT),
                        LV_PART_MAIN);
    }

    lv_label_set_text(ui_TempText2, "-- °C");
    lv_label_set_text(ui_BatteryText2, "--%");
    lv_arc_set_value(ui_Battery2, 0);
    lv_bar_set_value(ui_BatteryBar2, -10, LV_ANIM_ON);
    lv_image_set_src(ui_TPMSicon2, &ui_img_tpmsblack_png);
    lv_image_set_src(ui_BTicon2, &ui_img_btoff_png);
}

/**
 * @brief Update alert icons based on sensor alert flags
 * @param alertFront Front sensor alert status
 * @param alertRear Rear sensor alert status
 * @details If any sensor has alert flag set, blinks alert icons (ui_Alert1, ui_Alert2)
 *          at 250ms period. Shows idle icon when no alerts active.
 */
void UIBikeController::updateAlertIcons(bool alertFront, bool alertRear) {
    if (alertFront || alertRear) {
        // Blink: show alert when blink state is true, hide when false
        if (m_alertBlinkState) {
            lv_image_set_src(ui_Alert1, &ui_img_alert_png);
            lv_image_set_src(ui_Alert2, &ui_img_alert_png);
        } else {
            lv_image_set_src(ui_Alert1, &ui_img_idle_png);
            lv_image_set_src(ui_Alert2, &ui_img_idle_png);
        }
    } else {
        // No alert - show idle
        lv_image_set_src(ui_Alert1, &ui_img_idle_png);
        lv_image_set_src(ui_Alert2, &ui_img_idle_png);
    }
}

/**
 * @brief Update blink states for alerts and labels
 * @param currentTime Current timestamp in milliseconds
 * @details Toggles alert blink state every 250ms (for alert icons)
 *          and label blink state every 500ms (for unsynchronized sensor labels)
 */
void UIBikeController::updateAlertBlinkState(uint32_t currentTime) {
    // Toggle blink state every 250ms
    if (currentTime - m_lastBlinkTime >= 250) {
        m_alertBlinkState = !m_alertBlinkState;
        m_lastBlinkTime = currentTime;
    }

    // Toggle label blink state every 500ms
    if (currentTime - m_lastLabelBlinkTime >= 500) {
        m_labelBlinkState = !m_labelBlinkState;
        m_lastLabelBlinkTime = currentTime;
    }
}