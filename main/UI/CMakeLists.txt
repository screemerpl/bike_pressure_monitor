SET(SOURCES components/ui_comp_label1.c
    components/ui_comp_label2.c
    components/ui_comp.c
    screens/ui_Splash.c
    screens/ui_Main.c
    screens/ui_Black.c
    screens/ui_Pair.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_img_manager.c
    fastlz.c
    images/ui_img_1818877690.c
    images/ui_img_tpmsred_png.c
    images/ui_img_tpmsyellow_png.c
    images/ui_img_temp_png.c
    images/ui_img_btoff_png.c
    images/ui_img_bton_png.c
    images/ui_img_idle_png.c
    images/ui_img_alert_png.c
    images/ui_img_tpmsblack_png.c)

add_library(ui ${SOURCES})
