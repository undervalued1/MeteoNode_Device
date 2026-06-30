#ifndef WEATHER_FORECAST_H
#define WEATHER_FORECAST_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "lvgl.h"
#include "weather_images.h"
#include "globals/global_values.h"

#define WEATHER_UPDATE_INTERVAL 1800000UL   // 30 минут

struct DayWeather {
    int    wmo_code     = -1;
    float  temp_max     = NAN;
    float  temp_min     = NAN;
    float  precip_sum   = 0.0f;
    float  humidity     = NAN;
};

extern DayWeather forecast[3];
extern bool weather_valid;
extern unsigned long last_weather_ok;
extern unsigned long last_weather_attempt;   // ← новое

bool fetchAndParseWeather();
void updateWeatherUI();
void updateMainScreenWeather();

const lv_img_dsc_t* getWeatherIcon(int code);
const char* getWeatherDescription(int code);

#endif