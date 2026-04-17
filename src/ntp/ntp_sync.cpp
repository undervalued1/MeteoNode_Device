#include "ntp/ntp_sync.h"
#include <time.h>
#include "rtc/rtc.h"

void syncRTCWithNTP(RTCManager &rtc, const String &timezone)
{
    Serial.println("Starting NTP sync with timezone: " + timezone);

    // 1. Сначала подключаемся к NTP (получаем UTC время)
    configTime(0, 0, "pool.ntp.org", "time.google.com", "time.nist.gov");

    // 2. Применяем часовой пояс (это критично!)
    setenv("TZ", timezone.c_str(), 1);
    tzset();

    struct tm timeinfo = {0};
    const int timeout_ms = 15000;   // даём 15 секунд

    if (!getLocalTime(&timeinfo, timeout_ms))
    {
        Serial.println("Failed to obtain time → NTP sync unsuccessful");
        return;
    }

    // Проверка на разумность
    if (timeinfo.tm_year < (2024 - 1900))
    {
        Serial.println("NTP returned suspicious time → skipping");
        return;
    }

    // Создаём время для RTC (теперь это ЛОКАЛЬНОЕ время!)
    RtcDateTime rtcTime(
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday,
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec
    );

    rtc.setDateTime(rtcTime);

    Serial.println("RTC successfully synced with NTP (local time)");
    Serial.printf("Set time: %04d-%02d-%02d %02d:%02d:%02d  (%s)\n",
                  rtcTime.Year(), rtcTime.Month(), rtcTime.Day(),
                  rtcTime.Hour(), rtcTime.Minute(), rtcTime.Second(),
                  timezone.c_str());
}