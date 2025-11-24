/**
 * @file UICarController.cpp
 * @brief UI controller implementation for car main screen
 * @details Implements LVGL UI updates for TPMS sensor display on car main screen.
 *          Handles 4 sensors: pressure/temperature/battery, alert icons, and blink logic.
 */

#include "UICarController.h"
#include "State.h"
#include "UI/ui.h"
#include "UI/ui_themes.h"
#include "ui_theme_helper.h"
#include "lvgl.h"
#include "esp_log.h"
#include <cstdio>

UICarController &UICarController::instance() {
    static UICarController controller;
    return controller;
}

void UICarController::initializeLabels() {
    State &state = State::getInstance();
    lv_label_set_text(ui_UnitC, state.getPressureUnit().c_str());

    // Pressures
    lv_label_set_text(ui_PressureC1, "---");
    lv_label_set_text(ui_PressureC2, "---");
    lv_label_set_text(ui_PressureC3, "---");
    lv_label_set_text(ui_PressureC4, "---");

    // Temperatures
    lv_label_set_text(ui_TempTextC1, "-- °C");
    lv_label_set_text(ui_TempTextC2, "-- °C");
    lv_label_set_text(ui_TempTextC3, "-- °C");
    lv_label_set_text(ui_TempTextC4, "-- °C");

    // Battery text
    lv_label_set_text(ui_BatteryTextC1, "--%");
    lv_label_set_text(ui_BatteryTextC2, "--%");
    lv_label_set_text(ui_BatteryTextC3, "--%");
    lv_label_set_text(ui_BatteryTextC4, "--%");

    // Battery arcs
    lv_arc_set_value(ui_BatteryC1, 0);
    lv_arc_set_value(ui_BatteryC2, 0);
    lv_arc_set_value(ui_BatteryC3, 0);
    lv_arc_set_value(ui_BatteryC4, 0);

    // Bars
    lv_bar_set_value(ui_BatteryBarC1, -10, LV_ANIM_OFF);
    lv_bar_set_value(ui_BatteryBarC2, -10, LV_ANIM_OFF);
    lv_bar_set_value(ui_BatteryBarC3, -10, LV_ANIM_OFF);
    lv_bar_set_value(ui_BatteryBarC4, -10, LV_ANIM_OFF);

    // Icons
    lv_image_set_src(ui_TPMSiconC1, &ui_img_tpmsblack_png);
    lv_image_set_src(ui_TPMSiconC2, &ui_img_tpmsblack_png);
    lv_image_set_src(ui_TPMSiconC3, &ui_img_tpmsblack_png);
    lv_image_set_src(ui_TPMSiconC4, &ui_img_tpmsblack_png);
    lv_image_set_src(ui_BTiconC1, &ui_img_btoff_png);
    lv_image_set_src(ui_BTiconC2, &ui_img_btoff_png);
    lv_image_set_src(ui_BTiconC3, &ui_img_btoff_png);
    lv_image_set_src(ui_BTiconC4, &ui_img_btoff_png);

    // Alert icons default to idle
    lv_image_set_src(ui_AlertC1, &ui_img_idle_png);
    lv_image_set_src(ui_AlertC2, &ui_img_idle_png);

}

void UICarController::updateSensorUI(TPMSSensor *s1, TPMSSensor *s2, TPMSSensor *s3, TPMSSensor *s4,
                                     float ideal1, float ideal2, float ideal3, float ideal4,
                                     uint32_t currentTime) {
    bool a1 = false, a2 = false, a3 = false, a4 = false;

    if (s1) {
        updateSensorUIInternal(0, s1, ideal1, currentTime);
        a1 = s1->getAlert();
    } else {
        clearSensorUIInternal(0, true);
    }
    if (s2) {
        updateSensorUIInternal(1, s2, ideal2, currentTime);
        a2 = s2->getAlert();
    } else {
        clearSensorUIInternal(1, true);
    }
    if (s3) {
        updateSensorUIInternal(2, s3, ideal3, currentTime);
        a3 = s3->getAlert();
    } else {
        clearSensorUIInternal(2, true);
    }
    if (s4) {
        updateSensorUIInternal(3, s4, ideal4, currentTime);
        a4 = s4->getAlert();
    } else {
        clearSensorUIInternal(3, true);
    }

    updateAlertIcons(a1, a2, a3, a4);
}

void UICarController::updateSensorUIInternal(int idx, TPMSSensor *sensor, float idealPSI, uint32_t currentTime) {
    char buf[16];
    State &state = State::getInstance();

    lv_obj_t *pressureLabel = nullptr;
    lv_obj_t *tempLabel = nullptr;
    lv_obj_t *batteryLabel = nullptr;
    lv_obj_t *batteryBar = nullptr;
    lv_obj_t *batteryArc = nullptr;
    lv_obj_t *tpmsIcon = nullptr;
    lv_obj_t *btIcon = nullptr;

    switch (idx) {
        case 0:
            pressureLabel = ui_PressureC1; tempLabel = ui_TempTextC1; batteryLabel = ui_BatteryTextC1;
            batteryBar = ui_BatteryBarC1; batteryArc = ui_BatteryC1; tpmsIcon = ui_TPMSiconC1; btIcon = ui_BTiconC1;
            break;
        case 1:
            pressureLabel = ui_PressureC2; tempLabel = ui_TempTextC2; batteryLabel = ui_BatteryTextC2;
            batteryBar = ui_BatteryBarC2; batteryArc = ui_BatteryC2; tpmsIcon = ui_TPMSiconC2; btIcon = ui_BTiconC2;
            break;
        case 2:
            pressureLabel = ui_PressureC3; tempLabel = ui_TempTextC3; batteryLabel = ui_BatteryTextC3;
            batteryBar = ui_BatteryBarC3; batteryArc = ui_BatteryC3; tpmsIcon = ui_TPMSiconC3; btIcon = ui_BTiconC3;
            break;
        case 3:
            pressureLabel = ui_PressureC4; tempLabel = ui_TempTextC4; batteryLabel = ui_BatteryTextC4;
            batteryBar = ui_BatteryBarC4; batteryArc = ui_BatteryC4; tpmsIcon = ui_TPMSiconC4; btIcon = ui_BTiconC4;
            break;
        default:
            return;
    }

    // Pressure unit
    if (state.getPressureUnit() == "BAR") {
        snprintf(buf, sizeof(buf), "%.2f", sensor->getPressureBar());
    } else {
        snprintf(buf, sizeof(buf), "%.1f", sensor->getPressurePSI());
    }
    lv_label_set_text(pressureLabel, buf);
    lv_obj_set_style_text_color(pressureLabel, ui_theme_get_lv_color(UI_THEME_COLOR_TEXT), LV_PART_MAIN);

    snprintf(buf, sizeof(buf), "%.1f °C", sensor->getTemperatureC());
    lv_label_set_text(tempLabel, buf);

    snprintf(buf, sizeof(buf), "%d%%", sensor->getBatteryLevel());
    lv_label_set_text(batteryLabel, buf);

    lv_arc_set_value(batteryArc, static_cast<int>(sensor->getBatteryLevel()));
    lv_bar_set_value(batteryBar, static_cast<int>(sensor->getTemperatureC()), LV_ANIM_ON);

    if (sensor->getTemperatureC() < 10.0f) {
        lv_obj_set_style_bg_color(batteryBar, ui_theme_get_lv_color(UI_THEME_COLOR_STANDARD), LV_PART_MAIN);
        lv_obj_set_style_bg_color(batteryBar, ui_theme_get_lv_color(UI_THEME_COLOR_HOT), LV_PART_INDICATOR);
    } else {
        lv_obj_set_style_bg_color(batteryBar, ui_theme_get_lv_color(UI_THEME_COLOR_STANDARD), LV_PART_MAIN);
        lv_obj_set_style_bg_color(batteryBar, ui_theme_get_lv_color(UI_THEME_COLOR_BRIGHT), LV_PART_INDICATOR);
    }

    // Pressure indicator color
    if (sensor->getPressurePSI() < idealPSI * 0.75f) {
        lv_image_set_src(tpmsIcon, &ui_img_tpmsred_png);
    } else if (sensor->getPressurePSI() < idealPSI * 0.9f) {
        lv_image_set_src(tpmsIcon, &ui_img_tpmsyellow_png);
    } else {
        lv_image_set_src(tpmsIcon, &ui_img_tpmsblack_png);
    }

    // BLE status
    if (sensor->getTimestamp() + 200 < currentTime) {
        lv_image_set_src(btIcon, &ui_img_btoff_png);
    } else {
        lv_image_set_src(btIcon, &ui_img_bton_png);
    }
}

void UICarController::clearSensorUIInternal(int idx, bool applyBlink) {
    lv_obj_t *pressureLabel = nullptr;
    lv_obj_t *tempLabel = nullptr;
    lv_obj_t *batteryLabel = nullptr;
    lv_obj_t *batteryBar = nullptr;
    lv_obj_t *batteryArc = nullptr;
    lv_obj_t *tpmsIcon = nullptr;
    lv_obj_t *btIcon = nullptr;

    switch (idx) {
        case 0:
            pressureLabel = ui_PressureC1; tempLabel = ui_TempTextC1; batteryLabel = ui_BatteryTextC1;
            batteryBar = ui_BatteryBarC1; batteryArc = ui_BatteryC1; tpmsIcon = ui_TPMSiconC1; btIcon = ui_BTiconC1;
            break;
        case 1:
            pressureLabel = ui_PressureC2; tempLabel = ui_TempTextC2; batteryLabel = ui_BatteryTextC2;
            batteryBar = ui_BatteryBarC2; batteryArc = ui_BatteryC2; tpmsIcon = ui_TPMSiconC2; btIcon = ui_BTiconC2;
            break;
        case 2:
            pressureLabel = ui_PressureC3; tempLabel = ui_TempTextC3; batteryLabel = ui_BatteryTextC3;
            batteryBar = ui_BatteryBarC3; batteryArc = ui_BatteryC3; tpmsIcon = ui_TPMSiconC3; btIcon = ui_BTiconC3;
            break;
        case 3:
            pressureLabel = ui_PressureC4; tempLabel = ui_TempTextC4; batteryLabel = ui_BatteryTextC4;
            batteryBar = ui_BatteryBarC4; batteryArc = ui_BatteryC4; tpmsIcon = ui_TPMSiconC4; btIcon = ui_BTiconC4;
            break;
        default:
            return;
    }

    lv_label_set_text(pressureLabel, "---");
    if (applyBlink) {
        if (m_labelBlinkState) {
            lv_obj_set_style_text_color(pressureLabel, ui_theme_get_lv_color(UI_THEME_COLOR_TEXT), LV_PART_MAIN);
        } else {
            lv_obj_set_style_text_color(pressureLabel, ui_theme_get_lv_color(UI_THEME_COLOR_BACKGROUND), LV_PART_MAIN);
        }
    } else {
        lv_obj_set_style_text_color(pressureLabel, ui_theme_get_lv_color(UI_THEME_COLOR_TEXT), LV_PART_MAIN);
    }

    lv_label_set_text(tempLabel, "-- °C");
    lv_label_set_text(batteryLabel, "--%");
    lv_arc_set_value(batteryArc, 0);
    lv_bar_set_value(batteryBar, -10, LV_ANIM_ON);
    lv_image_set_src(tpmsIcon, &ui_img_tpmsblack_png);
    lv_image_set_src(btIcon, &ui_img_btoff_png);
}

void UICarController::updateAlertIcons(bool a1, bool a2, bool a3, bool a4) {
    // If any alert, blink, otherwise show idle
    bool any = a1 || a2 || a3 || a4;
    if (any) {
        if (m_alertBlinkState) {
            lv_image_set_src(ui_AlertC1, &ui_img_alert_png);
            lv_image_set_src(ui_AlertC2, &ui_img_alert_png);
          
        } else {
            lv_image_set_src(ui_AlertC1, &ui_img_idle_png);
            lv_image_set_src(ui_AlertC2, &ui_img_idle_png);
          
        }
    } else {
        lv_image_set_src(ui_AlertC1, &ui_img_idle_png);
        lv_image_set_src(ui_AlertC2, &ui_img_idle_png);
       
    }
}

void UICarController::updateAlertBlinkState(uint32_t currentTime) {
    if (currentTime - m_lastBlinkTime >= 250) {
        m_alertBlinkState = !m_alertBlinkState;
        m_lastBlinkTime = currentTime;
    }
    if (currentTime - m_lastLabelBlinkTime >= 500) {
        m_labelBlinkState = !m_labelBlinkState;
        m_lastLabelBlinkTime = currentTime;
    }
}
