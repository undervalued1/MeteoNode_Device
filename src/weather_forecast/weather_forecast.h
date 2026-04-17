#ifndef WEATHER_FORECAST_H
#define WEATHER_FORECAST_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "lvgl.h"
#include "weather_images.h"
#include "globals/global_values.h"

#define WEATHER_UPDATE_INTERVAL 1800000UL   // 30 мин

struct DayWeather {
    int    wmo_code     = -1;
    float  temp_max     = NAN;
    float  temp_min     = NAN;
    float  precip_sum   = 0.0f;
    float  humidity     = NAN;   // средняя влажность за день
};

extern DayWeather forecast[3];
extern bool weather_valid;
extern unsigned long last_weather_ok;

bool fetchAndParseWeather();
void updateWeatherUI();
void updateMainScreenWeather();   // добавлено
const lv_img_dsc_t* getWeatherIcon(int wmo);
const char* getWeatherDescription(int wmo);

#endif