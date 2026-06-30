#ifndef RTC_H
#define RTC_H

#include <Arduino.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

// Пины DS1302 для WT32-SC01 Plus
#define DS1302_CLK 14
#define DS1302_DAT 13
#define DS1302_RST 12

struct DateTime {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int dayOfWeek;
    
    DateTime() : year(2000), month(1), day(1), hour(0), minute(0), second(0), dayOfWeek(0) {}
};

class RTCManager {
private:
    ThreeWire myWire;
    RtcDS1302<ThreeWire> rtc;
    DateTime lastDateTime;
    bool initialized;
    
    DateTime convertRtcDateTime(const RtcDateTime& dt);
    
public:
    RTCManager();
    bool begin();
    DateTime getDateTime();
    
    // Обе перегрузки setDateTime
    void setDateTime(const DateTime& dt);               // ваша старая версия
    void setDateTime(const RtcDateTime& dt);            // ← новая, для NTP и библиотеки Makuna
    
    String getDateTimeString();
    String getTimeString();
    String getDateString();
    String getDayOfWeekString();
    void printToSerial();
    bool isInitialized() { return initialized; }
};

#endif