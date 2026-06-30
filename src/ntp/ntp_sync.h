#ifndef _NTP_SYNC_H
#define _NTP_SYNC_H

#include <Arduino.h>
#include "rtc/rtc.h"   // RTCManager

// Правильная сигнатура — полностью совпадает с реализацией в .cpp
// (const String& + значение по умолчанию в заголовке)
void syncRTCWithNTP(RTCManager &rtc, const String &timezone = "UTC0");

#endif