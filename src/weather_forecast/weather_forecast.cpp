#include "lvgl.h"           
#include "weather_forecast.h"
#include "ui.h"
#include "ui_WeatherScreen.h"
#include "ui_MainScreen.h"
#include "globals/global_values.h"

#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#define WEATHER_API_KEY "f7577840969b4571be9143550262206" 

DayWeather forecast[3];
bool weather_valid = false;
unsigned long last_weather_ok = 0;
unsigned long last_weather_attempt = 0;

// ==================== Маппинг WeatherAPI ====================
const lv_img_dsc_t* getWeatherIcon(int code) {
    if (code == 1000) return &clearSkyDayImg;                                       // Ясно
    if (code == 1003) return &fewCloudsDayImg;                                      // Переменная облачность
    if (code == 1006 || code == 1009) return &scatteredCloudsDayImg;                // Облачно / Пасмурно
    if (code == 1030 || code == 1135 || code == 1147) return &mistDayImg;           // Туман / Дымка
    if ((code >= 1063 && code <= 1201) || (code >= 1240 && code <= 1246)) return &rainDayImg; // Дождь
    if ((code >= 1066 && code <= 1225) || (code >= 1255 && code <= 1258)) return &snowDayImg; // Снег
    if (code == 1087 || (code >= 1273 && code <= 1282)) return &thunderstormDayImg; // Гроза
    
    return &clearSkyDayImg;
}

const char* getWeatherDescription(int code) {
    if (code == 1000) return "Clear";
    if (code == 1003) return "Partly Cloudy";
    if (code == 1006) return "Cloudy";
    if (code == 1009) return "Overcast";
    if (code == 1030) return "Mist";
    if (code == 1135) return "Fog";
    if (code == 1063 || code == 1183 || code == 1189 || code == 1195) return "Rain";
    if (code == 1066 || code == 1213 || code == 1219 || code == 1225) return "Snow";
    if (code == 1087) return "Thunderstorm";
    return "Weather";
}

bool fetchAndParseWeather() {
    unsigned long now = millis();

    // Защита от слишком частых запросов
    if (now - last_weather_attempt < 15000UL && weather_valid) {
        Serial.println("[Weather] Кэш ещё свежий — пропускаем запрос");
        return true;
    }

    last_weather_attempt = now;

    if (!wifiMgr.isConnected()) {
        Serial.println("[Weather] Нет Wi-Fi — используем кэш");
        return weather_valid;
    }

    LocationData loc = locationMgr.getLocation();
    if (loc.latitude == 0 && loc.longitude == 0) return weather_valid;

    String url = "http://api.weatherapi.com/v1/forecast.json";
    url += "?key=" + String(WEATHER_API_KEY);
    url += "&q=" + String(loc.latitude, 4) + "," + String(loc.longitude, 4);
    url += "&days=3&aqi=no&alerts=no&hour=0";   // ← важно для экономии

    Serial.println("[Weather] Запрос к WeatherAPI...");

    HTTPClient http;
    http.begin(url);
    int code = http.GET();

    if (code != 200) {
        Serial.printf("[Weather] HTTP ошибка: %d\n", code);
        http.end();
        return weather_valid;   // оставляем старый кэш
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.println("[Weather] JSON parse error");
        return weather_valid;
    }

    JsonArray days = doc["forecast"]["forecastday"];
    if (days.size() < 3) return weather_valid;

    for (int i = 0; i < 3; i++) {
        JsonObject dayObj = days[i]["day"];
        forecast[i].wmo_code   = dayObj["condition"]["code"].as<int>();
        forecast[i].temp_max   = dayObj["maxtemp_c"].as<float>();
        forecast[i].temp_min   = dayObj["mintemp_c"].as<float>();
        forecast[i].humidity   = dayObj["avghumidity"].as<float>();
        forecast[i].precip_sum = 0.0f;
    }

    weather_valid = true;
    last_weather_ok = now;
    Serial.println("[Weather] ✅ Данные успешно обновлены и закэшированы");
    return true;
}


void updateWeatherUI() {
    if (!weather_valid) return;

    char buf[32];
    time_t now = time(nullptr);
    struct tm* tm_info;

    // Сегодня
    lv_img_set_src(ui_TodayImg, getWeatherIcon(forecast[0].wmo_code));
    
    // Метка кэша
    const char* desc = getWeatherDescription(forecast[0].wmo_code);
    if (!wifiMgr.isConnected()) {
        lv_label_set_text(ui_weatherInfoToday, (String(desc) + " (кэш)").c_str());
    } else {
        lv_label_set_text(ui_weatherInfoToday, desc);
    }
    
    tm_info = localtime(&now);
    strftime(buf, sizeof(buf), "%a", tm_info);
    lv_label_set_text(ui_DayName, buf);

    if (!isnan(forecast[0].temp_max) && !isnan(forecast[0].temp_min)) {
        snprintf(buf, sizeof(buf), "%.0f/%.0f°C", forecast[0].temp_max, forecast[0].temp_min);
    } else {
        snprintf(buf, sizeof(buf), "--/--°C");
    }
    lv_label_set_text(ui_TodayTempLabel, buf);

    if (!isnan(forecast[0].humidity)) {
        snprintf(buf, sizeof(buf), "%.0f%%", forecast[0].humidity);
    } else {
        snprintf(buf, sizeof(buf), "--%%");
    }
    lv_label_set_text(ui_TodayHumLabel, buf);

    // Завтра
    lv_img_set_src(ui_TomorrowImg, getWeatherIcon(forecast[1].wmo_code));
    desc = getWeatherDescription(forecast[1].wmo_code);
    if (!wifiMgr.isConnected()) {
        lv_label_set_text(ui_weatherInfoTomorraw, (String(desc) + " (кэш)").c_str());
    } else {
        lv_label_set_text(ui_weatherInfoTomorraw, desc);
    }
    
    tm_info = localtime(&now);
    tm_info->tm_mday += 1;
    mktime(tm_info);
    strftime(buf, sizeof(buf), "%a", tm_info);
    lv_label_set_text(ui_DayName1, buf);

    if (!isnan(forecast[1].temp_max) && !isnan(forecast[1].temp_min)) {
        snprintf(buf, sizeof(buf), "%.0f/%.0f°C", forecast[1].temp_max, forecast[1].temp_min);
    } else {
        snprintf(buf, sizeof(buf), "--/--°C");
    }
    lv_label_set_text(ui_TomorrowTempLabel, buf);

    if (!isnan(forecast[1].humidity)) {
        snprintf(buf, sizeof(buf), "%.0f%%", forecast[1].humidity);
    } else {
        snprintf(buf, sizeof(buf), "--%%");
    }
    lv_label_set_text(ui_TomorrowHumLabel, buf);

    // Послезавтра
    lv_img_set_src(ui_TheDayAfterTomorrowImg, getWeatherIcon(forecast[2].wmo_code));
    desc = getWeatherDescription(forecast[2].wmo_code);
    if (!wifiMgr.isConnected()) {
        lv_label_set_text(ui_weatherInfoTheDayAfterTomorrow, (String(desc) + " (кэш)").c_str());
    } else {
        lv_label_set_text(ui_weatherInfoTheDayAfterTomorrow, desc);
    }
    
    tm_info = localtime(&now);
    tm_info->tm_mday += 2;
    mktime(tm_info);
    strftime(buf, sizeof(buf), "%a", tm_info);
    lv_label_set_text(ui_DayName2, buf);

    if (!isnan(forecast[2].temp_max) && !isnan(forecast[2].temp_min)) {
        snprintf(buf, sizeof(buf), "%.0f/%.0f°C", forecast[2].temp_max, forecast[2].temp_min);
    } else {
        snprintf(buf, sizeof(buf), "--/--°C");
    }
    lv_label_set_text(ui_AfterTomorrowTempLabel, buf);

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

    LocationData loc = locationMgr.getLocation();
    String locationText = loc.city;
    if (locationText.isEmpty()) {
        locationText = String(loc.latitude, 2) + ", " + String(loc.longitude, 2);
    }
    lv_label_set_text(ui_locationtext, locationText.c_str());

    lv_img_set_src(ui_WeatherOnMainScreenImg, getWeatherIcon(forecast[0].wmo_code));
    
    const char* desc = getWeatherDescription(forecast[0].wmo_code);
    if (!wifiMgr.isConnected()) {
        lv_label_set_text(ui_weatherInfoMainScreenLabel, (String(desc) + " (кэш)").c_str());
    } else {
        lv_label_set_text(ui_weatherInfoMainScreenLabel, desc);
    }

    if (!isnan(forecast[0].temp_max)) {
        snprintf(buf, sizeof(buf), "%.0f°C", forecast[0].temp_max);
    } else {
        snprintf(buf, sizeof(buf), "--°C");
    }
    lv_label_set_text(ui_tempAPIlbl, buf);

    if (!isnan(forecast[0].humidity)) {
        snprintf(buf, sizeof(buf), "%.0f%%", forecast[0].humidity);
    } else {
        snprintf(buf, sizeof(buf), "--%%");
    }
    lv_label_set_text(ui_humAPIlbl, buf);
}