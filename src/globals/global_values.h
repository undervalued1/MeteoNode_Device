#ifndef GLOBALS_VALUES_H
#define GLOBALS_VALUES_H

#include <Arduino.h>
#include "wifi_manager.h"
#include "location/location_manager.h"
#include "rtc/rtc.h"
#include "sensors/sensors.h"
#include "lgfx_config.h"

#define BUZZER_PIN  46

struct Thresholds {
    float max_temp = 32.0f;
    float min_temp = 16.0f;
    float max_hum  = 75.0f;
    float min_hum  = 25.0f;
    int max_co2    = 1200;
};

struct BrightnessSettings {
    int day_brightness = 100;
    int night_brightness = 20;
    bool auto_brightness = true;
};

extern Thresholds thresholds;
extern BrightnessSettings brightness;
extern LGFX tft;                
extern WifiManager  wifiMgr;      // Теперь объявление есть здесь
extern LocationManager locationMgr;
extern RTCManager   rtc;
extern Sensors      sensors;

void loadThresholds();
void saveThresholds();
void loadBrightness();
void saveBrightness();

// ==================== РАСПИСАНИЕ ЯРКОСТИ ====================
struct DaySchedule {
    int startHour = 7;
    int startMin  = 0;
    int endHour   = 22;
    int endMin    = 0;
};

struct BrightnessSchedule {
    DaySchedule days[7];      // 0 = Понедельник, 6 = Воскресенье
    bool useSchedule = false;
};

extern BrightnessSchedule brightnessSchedule;

void loadSchedule();
void saveSchedule();

#endif