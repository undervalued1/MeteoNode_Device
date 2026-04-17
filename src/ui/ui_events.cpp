#include "globals/global_values.h"
#include "ui.h"
#include "connections/wifi_manager.h"
#include <LovyanGFX.hpp>  // Для доступа к tft

// Глобальный объект из main.cpp
extern WifiManager wifiMgr;

// Обработчик переключателя Wi-Fi
void Wifisw_event_handler(lv_event_t * e)
{
    lv_obj_t * sw = lv_event_get_target(e);
    bool is_on = lv_obj_has_state(sw, LV_STATE_CHECKED);

    Serial.printf("Wifisw changed: %s\n", is_on ? "ON" : "OFF");

    if (is_on) {
        wifiMgr.begin();
        lv_obj_add_state(sw, LV_STATE_CHECKED);  // фиксируем визуально
    } else {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        lv_obj_clear_state(sw, LV_STATE_CHECKED);
    }

    // Обновляем иконку
    if (wifiMgr.isConnected()) {
        lv_obj_add_flag(ui_wifioffimg, LV_OBJ_FLAG_HIDDEN);
        Serial.println("WiFi connected → hide off icon");
    } else {
        lv_obj_clear_flag(ui_wifioffimg, LV_OBJ_FLAG_HIDDEN);
        Serial.println("WiFi disconnected → show off icon");
    }
}

// Обработчик кнопки сброса Wi-Fi
void ClearWifibtn_event_handler(lv_event_t * e)
{
    Serial.println("Clear WiFi settings pressed");
    wifiMgr.resetWiFi();  // стирает и перезагружает
}

//Обработчик слайдера яркости
void brightness_slider_event_handler(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int16_t value = lv_slider_get_value(slider);
    
    // Минимальная яркость - 5% (13 от 255, так как 255 * 0.05 = 12.75 → округляем до 13)
    const int MIN_BRIGHTNESS = 13;  // 5% яркости
    
    if (value < MIN_BRIGHTNESS) {
        value = MIN_BRIGHTNESS;
        lv_slider_set_value(slider, value, LV_ANIM_ON);
    }
    
    // Сохраняем в глобальную переменную
    screen_brightness = value;
    
    // Меняем яркость дисплея
    tft.setBrightness(value);
    
    // Обновляем текст с процентами
    if (ui_BrightnessText) {
        int percent = (value * 100) / 255;
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", percent);
        lv_label_set_text(ui_BrightnessText, buf);
    }
    
    // Отладочный вывод
    Serial.printf("Brightness changed: %d (%d%%)\n", value, (value * 100) / 255);
}