#ifndef _UI_EVENTS_H
#define _UI_EVENTS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// WiFi Screen
void Wifisw_event_handler(lv_event_t * e);
void ClearWifibtn_event_handler(lv_event_t * e);

// Brightness Screen
void brightness_slider_event_handler(lv_event_t * e);

// DateTime Screen
void DateTimeSaveBtn_event_handler(lv_event_t * e);
void DateTimeScreen_loaded_event_handler(lv_event_t * e);
void Calendar_event_handler(lv_event_t * e);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif