#include "location_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

LocationManager::LocationManager()
{
    location.valid = false;
}

bool LocationManager::begin()
{
    return true;
}

bool LocationManager::update()
{
    if (WiFi.status() != WL_CONNECTED)
        return false;

    HTTPClient http;
    http.begin("https://ipapi.co/json/");
    // Добавляем User-Agent для предотвращения ошибки 403
    http.setUserAgent("ESP32/1.0 (MeteoNodeDevice)");

    int httpCode = http.GET();

    if (httpCode != 200)
    {
        Serial.printf("[Location] HTTP error: %d\n", httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        Serial.println("[Location] JSON parse error");
        return false;
    }

    location.latitude  = doc["latitude"];
    location.longitude = doc["longitude"];
    location.timezone  = doc["timezone"].as<String>();
    location.city      = doc["city"].as<String>();   // извлекаем город
    location.valid     = true;

    Serial.println("[Location] Detected:");
    Serial.printf("  City: %s\n", location.city.c_str());
    Serial.printf("  Timezone: %s\n", location.timezone.c_str());
    Serial.printf("  Coordinates: %.2f, %.2f\n", location.latitude, location.longitude);

    return true;
}

LocationData LocationManager::getLocation()
{
    return location;
}