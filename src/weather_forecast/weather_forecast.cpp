// src/ui/weather_forecast/weather_forecast.cpp

#include "lvgl.h"           
#include "weather_forecast.h"
#include "ui.h"
#include "ui_WeatherScreen.h"
#include "ui_MainScreen.h"
#include "globals/global_values.h"   // для wifiMgr, locationMgr

DayWeather forecast[3];
bool weather_valid = false;
unsigned long last_weather_ok = 0;

const lv_img_dsc_t* getWeatherIcon(int wmo) {
    const lv_img_dsc_t* icon = &clearSkyDayImg;

    if      (wmo == 0)                  icon = &clearSkyDayImg;
    else if (wmo == 1 || wmo == 2)      icon = &fewCloudsDayImg;
    else if (wmo == 3)                  icon = &scatteredCloudsDayImg;
    else if (wmo >= 45 && wmo <= 48)    icon = &mistDayImg;
    else if (wmo >= 51 && wmo <= 65)    icon = &rainDayImg;
    else if (wmo >= 80 && wmo <= 82)    icon = &showerRainDayImg;
    else if ((wmo >= 71 && wmo <= 77) || (wmo >= 85 && wmo <= 86))
                                        icon = &snowDayImg;
    else if (wmo >= 95 && wmo <= 99)    icon = &thunderstormDayImg;

    return icon;
}

const char* getWeatherDescription(int wmo) {
    switch (wmo) {
        case 0:     return "Clear sky";
        case 1:     return "Mainly clear";
        case 2:     return "Partly cloudy";
        case 3:     return "Overcast";
        case 45:    
        case 48:    return "Fog";
        case 51:    
        case 53:    
        case 55:    return "Drizzle";
        case 56:    
        case 57:    return "Freezing drizzle";
        case 61:    
        case 63:    
        case 65:    return "Rain";
        case 66:    
        case 67:    return "Freezing rain";
        case 71:    
        case 73:    
        case 75:    return "Snow";
        case 77:    return "Snow grains";
        case 80:    
        case 81:    
        case 82:    return "Rain showers";
        case 85:    
        case 86:    return "Snow showers";
        case 95:    return "Thunderstorm";
        case 96:    
        case 99:    return "Thunderstorm hail";
        default:    return "—";
    }
}

bool fetchAndParseWeather() {
    if (!wifiMgr.isConnected()) return false;

    LocationData loc = locationMgr.getLocation();
    if (loc.latitude == 0 && loc.longitude == 0) return false;

    String url = "https://api.open-meteo.com/v1/forecast";
    url += "?latitude="   + String(loc.latitude, 5);
    url += "&longitude="  + String(loc.longitude, 5);
    url += "&timezone=auto&forecast_days=3";
    url += "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_sum,relative_humidity_2m_mean";

    HTTPClient http;
    http.begin(url);
    int code = http.GET();

    if (code <= 0 || code != 200) {
        Serial.printf("[Weather] HTTP %d\n", code);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.print("[Weather] JSON err: "); Serial.println(err.c_str());
        return false;
    }

    JsonArray times    = doc["daily"]["time"];
    JsonArray codes    = doc["daily"]["weather_code"];
    JsonArray tmax     = doc["daily"]["temperature_2m_max"];
    JsonArray tmin     = doc["daily"]["temperature_2m_min"];
    JsonArray precip   = doc["daily"]["precipitation_sum"];
    JsonArray humidity = doc["daily"]["relative_humidity_2m_mean"];

    if (times.size() < 3) return false;

    for (int i = 0; i < 3; i++) {
        forecast[i].wmo_code   = codes[i].as<int>();
        forecast[i].temp_max   = tmax[i].as<float>();
        forecast[i].temp_min   = tmin[i].as<float>();
        forecast[i].precip_sum = precip[i].as<float>();
        forecast[i].humidity   = humidity[i].as<float>();
    }

    weather_valid = true;
    last_weather_ok = millis();
    Serial.println("[Weather] Updated");
    return true;
}

void updateWeatherUI() {
    if (!weather_valid) return;

    char buf[32];
    time_t now = time(nullptr);
    struct tm* tm_info;

    // Сегодня
    lv_img_set_src(ui_TodayImg, getWeatherIcon(forecast[0].wmo_code));
    lv_label_set_text(ui_weatherInfoToday, getWeatherDescription(forecast[0].wmo_code));
    tm_info = localtime(&now);
    strftime(buf, sizeof(buf), "%a", tm_info);
    lv_label_set_text(ui_DayName, buf);

    // Температура сегодня (макс/мин)
    if (!isnan(forecast[0].temp_max) && !isnan(forecast[0].temp_min)) {
        snprintf(buf, sizeof(buf), "%.0f/%.0f°C", forecast[0].temp_max, forecast[0].temp_min);
    } else {
        snprintf(buf, sizeof(buf), "--/--°C");
    }
    lv_label_set_text(ui_TodayTempLabel, buf);

    // Влажность сегодня
    if (!isnan(forecast[0].humidity)) {
        snprintf(buf, sizeof(buf), "%.0f%%", forecast[0].humidity);
    } else {
        snprintf(buf, sizeof(buf), "--%%");
    }
    lv_label_set_text(ui_TodayHumLabel, buf);

    // Завтра
    lv_img_set_src(ui_TomorrowImg, getWeatherIcon(forecast[1].wmo_code));
    lv_label_set_text(ui_weatherInfoTomorraw, getWeatherDescription(forecast[1].wmo_code));
    tm_info = localtime(&now);
    tm_info->tm_mday += 1;
    mktime(tm_info);
    strftime(buf, sizeof(buf), "%a", tm_info);
    lv_label_set_text(ui_DayName1, buf);

    // Температура завтра
    if (!isnan(forecast[1].temp_max) && !isnan(forecast[1].temp_min)) {
        snprintf(buf, sizeof(buf), "%.0f/%.0f°C", forecast[1].temp_max, forecast[1].temp_min);
    } else {
        snprintf(buf, sizeof(buf), "--/--°C");
    }
    lv_label_set_text(ui_TomorrowTempLabel, buf);

    // Влажность завтра
    if (!isnan(forecast[1].humidity)) {
        snprintf(buf, sizeof(buf), "%.0f%%", forecast[1].humidity);
    } else {
        snprintf(buf, sizeof(buf), "--%%");
    }
    lv_label_set_text(ui_TomorrowHumLabel, buf);

    // Послезавтра
    lv_img_set_src(ui_TheDayAfterTomorrowImg, getWeatherIcon(forecast[2].wmo_code));
    lv_label_set_text(ui_weatherInfoTheDayAfterTomorrow, getWeatherDescription(forecast[2].wmo_code));
    tm_info = localtime(&now);
    tm_info->tm_mday += 2;
    mktime(tm_info);
    strftime(buf, sizeof(buf), "%a", tm_info);
    lv_label_set_text(ui_DayName2, buf);

    // Температура послезавтра
    if (!isnan(forecast[2].temp_max) && !isnan(forecast[2].temp_min)) {
        snprintf(buf, sizeof(buf), "%.0f/%.0f°C", forecast[2].temp_max, forecast[2].temp_min);
    } else {
        snprintf(buf, sizeof(buf), "--/--°C");
    }
    lv_label_set_text(ui_AfterTomorrowTempLabel, buf);

    // Влажность послезавтра
    if (!isnan(forecast[2].humidity)) {
        snprintf(buf, sizeof(buf), "%.0f%%", forecast[2].humidity);
    } else {
        snprintf(buf, sizeof(buf), "--%%");
    }
    lv_label_set_text(ui_AfterTomorrowHumLabel, buf);
}

void updateMainScreenWeather() {
    if (!weather_valid) return;

    char buf[32];

    // 1. Локация
    LocationData loc = locationMgr.getLocation();
    // Если есть название города — используем его, иначе формируем из координат
    String locationText = loc.city;
    if (locationText.isEmpty()) {
        locationText = String(loc.latitude, 2) + ", " + String(loc.longitude, 2);
    }
    lv_label_set_text(ui_locationtext, locationText.c_str());

    // 2. Иконка погоды сегодня
    lv_img_set_src(ui_WeatherOnMainScreenImg, getWeatherIcon(forecast[0].wmo_code));

    // 3. Описание погоды сегодня
    lv_label_set_text(ui_weatherInfoMainScreenLabel, getWeatherDescription(forecast[0].wmo_code));

    // 4. Температура сегодня (показываем максимум)
    if (!isnan(forecast[0].temp_max)) {
        snprintf(buf, sizeof(buf), "%.0f°C", forecast[0].temp_max);
    } else {
        snprintf(buf, sizeof(buf), "--°C");
    }
    lv_label_set_text(ui_tempAPIlbl, buf);

    // 5. Влажность сегодня
    if (!isnan(forecast[0].humidity)) {
        snprintf(buf, sizeof(buf), "%.0f%%", forecast[0].humidity);
    } else {
        snprintf(buf, sizeof(buf), "--%%");
    }
    lv_label_set_text(ui_humAPIlbl, buf);
}