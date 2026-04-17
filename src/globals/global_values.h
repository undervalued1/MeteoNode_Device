#ifndef GLOBALS_VALUES_H
#define GLOBALS_VALUES_H

#include "wifi_manager.h"
#include "location/location_manager.h"
#include "rtc/rtc.h"
#include "sensors/sensors.h"
#include "lgfx_config.h"


extern LGFX tft;                
extern int screen_brightness;   

extern WifiManager  wifiMgr;
extern LocationManager locationMgr;
extern RTCManager   rtc;
extern Sensors      sensors;

extern int screen_brightness;

#endif