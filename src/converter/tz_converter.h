// tz_converter.h
#ifndef TZ_CONVERTER_H
#define TZ_CONVERTER_H

#include <Arduino.h>

class TimezoneConverter {
public:
    static String toPosix(const String& ianaTimezone) {
        // Россия
        if (ianaTimezone == "Europe/Moscow") return "MSK-3";
        if (ianaTimezone == "Europe/Samara") return "SAMT-4";
        if (ianaTimezone == "Asia/Yekaterinburg") return "YEKT-5";  // UTC+5
        if (ianaTimezone == "Asia/Omsk") return "OMST-6";
        if (ianaTimezone == "Asia/Krasnoyarsk") return "KRAT-7";
        if (ianaTimezone == "Asia/Irkutsk") return "IRKT-8";
        if (ianaTimezone == "Asia/Yakutsk") return "YAKT-9";
        if (ianaTimezone == "Asia/Vladivostok") return "VLAT-10";
        if (ianaTimezone == "Asia/Magadan") return "MAGT-11";
        if (ianaTimezone == "Asia/Kamchatka") return "PETT-12";
        if (ianaTimezone == "Europe/Kaliningrad") return "EET-2";
        
        // Основные мировые зоны
        if (ianaTimezone == "UTC" || ianaTimezone == "Etc/UTC") return "UTC0";
        if (ianaTimezone == "GMT" || ianaTimezone == "Etc/GMT") return "GMT0";
        
        // Европа
        if (ianaTimezone == "Europe/London") return "GMT0BST-1,M3.5.0/1,M10.5.0";
        if (ianaTimezone == "Europe/Paris") return "CET-1CEST-2,M3.5.0/2,M10.5.0/3";
        if (ianaTimezone == "Europe/Berlin") return "CET-1CEST-2,M3.5.0/2,M10.5.0/3";
        if (ianaTimezone == "Europe/Rome") return "CET-1CEST-2,M3.5.0/2,M10.5.0/3";
        if (ianaTimezone == "Europe/Madrid") return "CET-1CEST-2,M3.5.0/2,M10.5.0/3";
        
        // Америка
        if (ianaTimezone == "America/New_York") return "EST5EDT,M3.2.0/2,M11.1.0";
        if (ianaTimezone == "America/Chicago") return "CST6CDT,M3.2.0/2,M11.1.0";
        if (ianaTimezone == "America/Denver") return "MST7MDT,M3.2.0/2,M11.1.0";
        if (ianaTimezone == "America/Los_Angeles") return "PST8PDT,M3.2.0/2,M11.1.0";
        if (ianaTimezone == "America/Toronto") return "EST5EDT,M3.2.0/2,M11.1.0";
        if (ianaTimezone == "America/Vancouver") return "PST8PDT,M3.2.0/2,M11.1.0";
        
        // Азия
        if (ianaTimezone == "Asia/Tokyo") return "JST-9";
        if (ianaTimezone == "Asia/Shanghai") return "CST-8";
        if (ianaTimezone == "Asia/Hong_Kong") return "HKT-8";
        if (ianaTimezone == "Asia/Singapore") return "SGT-8";
        if (ianaTimezone == "Asia/Dubai") return "GST-4";
        if (ianaTimezone == "Asia/Kolkata") return "IST-5:30";
        if (ianaTimezone == "Asia/Bangkok") return "ICT-7";
        
        // Австралия
        if (ianaTimezone == "Australia/Sydney") return "AEST-10AEDT-11,M10.1.0/2,M4.1.0/3";
        if (ianaTimezone == "Australia/Perth") return "AWST-8";
        
        // Если не нашли, пробуем извлечь смещение
        return extractOffsetFromIANA(ianaTimezone);
    }

private:
    static String extractOffsetFromIANA(const String& iana) {
        // Попытка извлечь смещение из формата "Etc/GMT+5" или подобных
        if (iana.startsWith("Etc/GMT")) {
            String offset = iana.substring(7);
            if (offset.length() > 0) {
                int hours = offset.toInt();
                // В Etc/GMT знак инвертирован!
                return "UTC" + String(-hours);
            }
        }
        
        // По умолчанию возвращаем как есть (может не работать)
        Serial.printf("⚠️ Unknown timezone: %s, using UTC0\n", iana.c_str());
        return "UTC0";
    }
};

#endif