#include "globals/global_values.h"
#include "ui.h"
#include "ui_events.h"
#include "connections/wifi_manager.h"
#include "rtc/rtc.h"
#include <LovyanGFX.hpp>
#include <WiFi.h>

// Глобальные объекты из main.cpp
extern WifiManager wifiMgr;
extern RTCManager rtc;
extern LGFX tft;
extern int screen_brightness;

// ==============================================
//  WiFi Screen Event Handlers
// ==============================================

void Wifisw_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    lv_obj_t * sw = lv_event_get_target(e);
    bool is_on = lv_obj_has_state(sw, LV_STATE_CHECKED);

    Serial.printf("Wifisw changed: %s\n", is_on ? "ON" : "OFF");

    if (is_on) {
        wifiMgr.begin();
        lv_obj_add_state(sw, LV_STATE_CHECKED);
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

void ClearWifibtn_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    Serial.println("Clear WiFi settings pressed");
    wifiMgr.resetWiFi();  // стирает и перезагружает
}

// ==============================================
//  Brightness Screen Event Handler
// ==============================================

void brightness_slider_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_VALUE_CHANGED) return;
    
    lv_obj_t * slider = lv_event_get_target(e);
    int16_t value = lv_slider_get_value(slider);
    
    // Минимальная яркость - 5% (13 от 255, так как 255 * 0.05 = 12.75 → округляем до 13)
    const int MIN_BRIGHTNESS = 13;
    
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
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", percent);
        lv_label_set_text(ui_BrightnessText, buf);
    }
    
    Serial.printf("Brightness changed: %d (%d%%)\n", value, (value * 100) / 255);
}

// ==============================================
//  DateTime Screen Event Handlers
// ==============================================

void DateTimeSaveBtn_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    lv_calendar_date_t date = {0};

    // Правильный способ получения даты из календаря в LVGL 8.3
    const lv_calendar_date_t * today = lv_calendar_get_today_date(ui_Calendar);
    if (today != NULL) {
        date = *today;
    }

    uint16_t hour = lv_roller_get_selected(ui_hourRoller);
    uint16_t minute = lv_roller_get_selected(ui_minuteRoller);

    Serial.printf("[DateTime] Saving: %04d-%02d-%02d %02d:%02d\n", 
                  date.year, date.month, date.day, hour, minute);

    // Создаем структуру DateTime
    DateTime newDateTime;
    newDateTime.year = date.year;
    newDateTime.month = date.month;
    newDateTime.day = date.day;
    newDateTime.hour = (int)hour;
    newDateTime.minute = (int)minute;
    newDateTime.second = 0;
    newDateTime.dayOfWeek = 0; // RTC сам вычислит день недели

    // Вызываем setDateTime (возвращает void)
    rtc.setDateTime(newDateTime);

    // Проверим, установилось ли время
    DateTime verifyTime = rtc.getDateTime();
    if (verifyTime.year > 2023 && verifyTime.month == newDateTime.month && verifyTime.day == newDateTime.day)
    {
        lv_label_set_text(ui_Label2, "✓ Saved");
        lv_obj_set_style_text_color(ui_Label2, lv_color_hex(0x00FF88), LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_timer_create([](lv_timer_t* t) {
            _ui_screen_change(&ui_OptionsScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, NULL);
            lv_timer_del(t);
        }, 1300, NULL);
    }
    else
    {
        lv_label_set_text(ui_Label2, "Save Error");
        lv_obj_set_style_text_color(ui_Label2, lv_color_hex(0xFF4444), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void DateTimeScreen_loaded_event_handler(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_LOADED) return;

    // ИСПРАВЛЯЕМ ДИАПАЗОН МИНУТ ЕСЛИ НУЖНО
    uint16_t option_count = lv_roller_get_option_cnt(ui_minuteRoller);
    if (option_count != 60) {
        String options = "";
        for (int i = 0; i < 60; i++) {
            if (i < 10) options += "0";
            options += String(i);
            if (i < 59) options += "\n";
        }
        lv_roller_set_options(ui_minuteRoller, options.c_str(), LV_ROLLER_MODE_NORMAL);
        lv_roller_set_visible_row_count(ui_minuteRoller, 5);
        Serial.println("Minute roller fixed to 0-59 range");
    }

    DateTime now = rtc.getDateTime();

    // Заполняем ролики
    lv_roller_set_selected(ui_hourRoller, now.hour, LV_ANIM_OFF);
    lv_roller_set_selected(ui_minuteRoller, now.minute, LV_ANIM_OFF);

    // Заполняем календарь
    lv_calendar_set_today_date(ui_Calendar, now.year, now.month, now.day);
    lv_calendar_set_showed_date(ui_Calendar, now.year, now.month);
    
    // Устанавливаем выделенную дату
    lv_calendar_date_t highlighted_date;
    highlighted_date.year = now.year;
    highlighted_date.month = now.month;
    highlighted_date.day = now.day;
    lv_calendar_set_highlighted_dates(ui_Calendar, &highlighted_date, 1);

    // Сбрасываем текст кнопки
    lv_label_set_text(ui_Label2, "Save");
    lv_obj_set_style_text_color(ui_Label2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    if (wifiMgr.isConnected()) {
        Serial.println("DateTimeScreen: NTP synchronized time loaded");
    } else {
        Serial.println("DateTimeScreen: Manual time setting mode");
    }
}


void Calendar_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_calendar_date_t date;
        if (lv_calendar_get_pressed_date(ui_Calendar, &date)) {
            Serial.printf("Date selected: %04d-%02d-%02d\n", date.year, date.month, date.day);
            
            // Устанавливаем выбранную дату как текущую для сохранения
            lv_calendar_set_today_date(ui_Calendar, date.year, date.month, date.day);
            
            // Снимаем подсветку со всех дат и подсвечиваем выбранную
            lv_calendar_set_highlighted_dates(ui_Calendar, &date, 1);
            
            // Опционально: показываем выбранную дату в лейбле
            char dateStr[32];
            snprintf(dateStr, sizeof(dateStr), "Selected: %02d.%02d.%04d", 
                     date.day, date.month, date.year);
            lv_label_set_text(ui_Label2, dateStr);
        }
    }
}