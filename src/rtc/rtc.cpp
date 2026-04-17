#include "rtc.h"

static const char* DAYS_OF_WEEK[] = {"Sunday", "Monday", "Tuesday", "Wednesday", 
                                      "Thursday", "Friday", "Saturday"};

RTCManager::RTCManager() 
    : myWire(DS1302_DAT, DS1302_CLK, DS1302_RST)
    , rtc(myWire)
    , initialized(false) 
{
}

DateTime RTCManager::convertRtcDateTime(const RtcDateTime& dt) {
    DateTime result;
    result.year = dt.Year();
    result.month = dt.Month();
    result.day = dt.Day();
    result.hour = dt.Hour();
    result.minute = dt.Minute();
    result.second = dt.Second();
    result.dayOfWeek = dt.DayOfWeek();
    return result;
}

bool RTCManager::begin() {
    Serial.println("\n=== Initializing RTC DS1302 ===");
    
    rtc.Begin();
    
    if (rtc.GetIsWriteProtected()) {
        Serial.println("Removing write protection...");
        rtc.SetIsWriteProtected(false);
    }
    
    if (!rtc.GetIsRunning()) {
        Serial.println("Starting RTC...");
        rtc.SetIsRunning(true);
    }
    
    if (!rtc.IsDateTimeValid()) {
        Serial.println("RTC invalid - setting compile time");
        RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
        rtc.SetDateTime(compiled);
    }
    
    initialized = true;
    printToSerial();
    return true;
}

DateTime RTCManager::getDateTime() {
    if (!initialized) return DateTime();
    RtcDateTime now = rtc.GetDateTime();
    lastDateTime = convertRtcDateTime(now);
    return lastDateTime;
}

// ────────────── Старая версия (для struct DateTime) ──────────────
void RTCManager::setDateTime(const DateTime& dt) {
    if (!initialized) return;
    RtcDateTime newTime(dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
    rtc.SetDateTime(newTime);
}

// ────────────── Новая версия (для RtcDateTime) ──────────────
void RTCManager::setDateTime(const RtcDateTime& dt) {
    if (!initialized) return;
    rtc.SetDateTime(dt);
}

String RTCManager::getDateTimeString() {
    if (!initialized) return "RTC Error";
    RtcDateTime now = rtc.GetDateTime();
    char buf[30];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             now.Year(), now.Month(), now.Day(),
             now.Hour(), now.Minute(), now.Second());
    return String(buf);
}

String RTCManager::getTimeString() {
    if (!initialized) return "Error";
    RtcDateTime now = rtc.GetDateTime();
    char buf[10];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             now.Hour(), now.Minute(), now.Second());
    return String(buf);
}

String RTCManager::getDateString() {
    if (!initialized) return "Error";
    RtcDateTime now = rtc.GetDateTime();
    char buf[12];
    snprintf(buf, sizeof(buf), "%02d.%02d.%04d",
             now.Day(), now.Month(), now.Year());
    return String(buf);
}

String RTCManager::getDayOfWeekString() {
    if (!initialized) return "Error";
    RtcDateTime now = rtc.GetDateTime();
    int dow = now.DayOfWeek();
    if (dow >= 0 && dow <= 6) {
        return String(DAYS_OF_WEEK[dow]);
    }
    return "Unknown";
}

void RTCManager::printToSerial() {
    if (!initialized) {
        Serial.println("RTC not initialized!");
        return;
    }
    
    RtcDateTime now = rtc.GetDateTime();
    Serial.println("┌─────────────────────────┐");
    Serial.println("│       RTC DS1302        │");
    Serial.println("├─────────────────────────┤");
    Serial.printf("│ %s                  │\n", getDayOfWeekString().c_str());
    Serial.printf("│ Date: %02d.%02d.%04d         │\n", 
                  now.Day(), now.Month(), now.Year());
    Serial.printf("│ Time: %02d:%02d:%02d            │\n", 
                  now.Hour(), now.Minute(), now.Second());
    Serial.println("└─────────────────────────┘\n");
}