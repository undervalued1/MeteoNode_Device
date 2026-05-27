#include <Arduino.h>
#include <time.h>
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include <Wire.h>


#include "weather_forecast/weather_forecast.h"

#include "ntp/ntp_sync.h"
#include "location/location_manager.h"
#include "converter/tz_converter.h"  

#include "globals/lgfx_config.h"
#include "globals/global_values.h" 
#include "ui.h"
#include "sensors/sensors.h"
#include "connections/wifi_manager.h"
#include "rtc/rtc.h"

#include "button/button.h"


#define LGFX_USE_V1

//  ====================== Глобальные объекты ======================
LGFX tft;
int screen_brightness = 255;

Sensors      sensors;
WifiManager  wifiMgr;
RTCManager   rtc;
LocationManager locationMgr;
Button       btn;

bool ntpSynced = false;
bool locationFetched = false;
bool locationAttempted = false;  

unsigned long lastNTPSync = 0;
const unsigned long NTP_SYNC_INTERVAL = 86400000UL; // 24 часа




//  ====================== Проверка RTC ======================
bool isRTCInvalid()
{
    DateTime dt = rtc.getDateTime();
    
    // Проверяем несколько условий:
    // 1. Год вне разумного диапазона
    if (dt.year < 2023 || dt.year > 2100) 
        return true;
    
    // 2. Месяц не в диапазоне 1-12
    if (dt.month < 1 || dt.month > 12)
        return true;
    
    // 3. День не в диапазоне 1-31
    if (dt.day < 1 || dt.day > 31)
        return true;
    
    // 4. Часы/минуты/секунды вне диапазона
    if (dt.hour > 23 || dt.minute > 59 || dt.second > 59)
        return true;
    
    // 5. Специальный случай: время после 1 января 2024 (слишком старое)
    // Например, если RTC сбросился на 2000-01-01
    if (dt.year == 2000 && dt.month == 1 && dt.day == 1)
        return true;
    
    return false;
}


// ====================== LVGL ======================
static const uint32_t screenWidth  = 480;
static const uint32_t screenHeight = 320;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 80];

// LVGL callbacks
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
    tft.endWrite();
    lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t x, y;
    if (tft.getTouch(&x, &y))
    {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = x;
        data->point.y = y;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

// Display init
void initDisplay()
{
    tft.begin();
    tft.setRotation(1);
    tft.setBrightness(screen_brightness);
    tft.fillScreen(TFT_BLACK);
}

// Обновление датчиков и времени
// Обновление датчиков и времени
void UpdateSensorsAndTime()
{
    SensorData data = sensors.read();
    DateTime dt = rtc.getDateTime();
    char buf[32];  // увеличил размер буфера на всякий случай

    // ====================== ВРЕМЯ ======================
    snprintf(buf, sizeof(buf), "%02d", dt.hour);
    lv_label_set_text(ui_hour, buf);
    lv_label_set_text(ui_Hourlbl, buf);
    lv_label_set_text(ui_Hourlbl1, buf);

    snprintf(buf, sizeof(buf), "%02d", dt.minute);
    lv_label_set_text(ui_minute, buf);
    lv_label_set_text(ui_Minutlbl, buf);
    lv_label_set_text(ui_Minutlbl1, buf);

    lv_label_set_text(ui_datetext, rtc.getDateString().c_str());
    lv_label_set_text(ui_Datelbl, rtc.getDateString().c_str());
    lv_label_set_text(ui_Datelbl1, rtc.getDateString().c_str());

    // ====================== ДАТЧИКИ ======================
    if (data.isValid)
    {
        // Температура и влажность
        snprintf(buf, sizeof(buf), "%.1f", data.temperature); 
        lv_label_set_text(ui_Templbl, buf);

        snprintf(buf, sizeof(buf), "%.0f", data.humidity); 
        lv_label_set_text(ui_Humlbl, buf);

        // Давление и высота
        snprintf(buf, sizeof(buf), "%.0f hPa", data.pressure); 
        lv_label_set_text(ui_Pressurelbl, buf);

        snprintf(buf, sizeof(buf), "%.0f m", data.altitude); 
        lv_label_set_text(ui_AltitudeAboveSeaLbl, buf);

        // ENS160
        snprintf(buf, sizeof(buf), "%d ppm", data.co2); 
        lv_label_set_text(ui_CO2lbl, buf);

        snprintf(buf, sizeof(buf), "%d ppb", data.tvoc); 
        lv_label_set_text(ui_TVOClbl, buf);

        // ====================== AQI ARC ======================
        if (data.aqi >= 1 && data.aqi <= 5)
        {
            lv_arc_set_value(ui_AQIArc, data.aqi);
            
            // === Изменено: теперь показывает число + качество ===
            char aqiText[20];
            snprintf(aqiText, sizeof(aqiText), "%d - %s", 
                     data.aqi, sensors.getAQIString(data.aqi).c_str());
            
            lv_label_set_text(ui_AQILabel, aqiText);

            // Цвет дуги
            lv_color_t color;
            switch(data.aqi)
            {
                case 1:  color = lv_color_hex(0x00FF00); break; // Excellent — зелёный
                case 2:  color = lv_color_hex(0xAAFF00); break; // Good
                case 3:  color = lv_color_hex(0xFFFF00); break; // Moderate — жёлтый
                case 4:  color = lv_color_hex(0xFF8800); break; // Poor — оранжевый
                case 5:  color = lv_color_hex(0xFF0000); break; // Unhealthy — красный
                default: color = lv_color_hex(0x888888);
            }
            lv_obj_set_style_arc_color(ui_AQIArc, color, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        }
        else
        {
            lv_arc_set_value(ui_AQIArc, 0);
            lv_label_set_text(ui_AQILabel, "--");
        }
    }
    else
    {
        lv_arc_set_value(ui_AQIArc, 0);
        lv_label_set_text(ui_AQILabel, "No data");
    }
}

// WiFi UI
void UpdateWiFiStatus()
{
    bool connected = wifiMgr.isConnected();
    if (connected)
    {
        lv_obj_add_state(ui_Wifisw, LV_STATE_CHECKED);
        lv_obj_add_flag(ui_wifioffimg, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_clear_state(ui_Wifisw, LV_STATE_CHECKED);
        lv_obj_clear_flag(ui_wifioffimg, LV_OBJ_FLAG_HIDDEN);
    }
}

// SETUP
void setup()
{
    Serial.begin(115200); delay(500);
    initDisplay();
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 80);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth; disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER; indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    ui_init();

    sensors.begin();
    wifiMgr.setSensors(&sensors);
    
    rtc.begin();
    wifiMgr.begin();
    locationMgr.begin();
    
    btn.begin();
    Serial.println("System started with physical button on GPIO21");

    Serial.println("System started");
}

// ====================== УПРАВЛЕНИЕ ЭКРАНАМИ ЧЕРЕЗ КНОПКУ ======================
static lv_obj_t* screens[] = {
    ui_MainScreen,
    ui_SensorsDataScreen,
    ui_WeatherScreen,
    ui_WiFiScreen
};

static const int totalScreens = sizeof(screens) / sizeof(screens[0]);
static int currentScreenIndex = 0;

void handleButtonPress()
{
    if (btn.isDoubleClicked())
    {
        Serial.println("Button: Double click → Main Screen");
        _ui_screen_change(&ui_MainScreen, LV_SCR_LOAD_ANIM_FADE_ON, 250, 0, NULL);
        currentScreenIndex = 0;
    }
    else if (btn.isClicked())
    {
        Serial.println("Button: Single click → Next Screen");
        
        currentScreenIndex = (currentScreenIndex + 1) % totalScreens;
        
        _ui_screen_change(&screens[currentScreenIndex], 
                         LV_SCR_LOAD_ANIM_MOVE_LEFT, 320, 0, NULL);
    }
    else if (btn.isLongPressed())
    {
        Serial.println("Button: Long press → Options Screen");
        _ui_screen_change(&ui_OptionsScreen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, NULL);
    }
}

// LOOP
void loop()
{
    static uint32_t last_tick          = 0;
    static uint32_t last_sensor        = 0;
    static uint32_t last_wifi          = 0;
    static uint32_t last_ntp_attempt   = 0;
    static uint32_t last_weather_try   = 0;

    static bool firstSyncDone = false;

    uint32_t now = millis();

    wifiMgr.update();

    // ====================== ПОСЛЕДОВАТЕЛЬНОСТЬ: WiFi → Локация → NTP ======================
    if (wifiMgr.isConnected())
    {
        // 1. ПОЛУЧЕНИЕ ЛОКАЦИИ (только если не пытались)
        if (!locationAttempted)
        {
            Serial.println("Fetching location from ipapi.co...");

            if (locationMgr.update())
            {
                LocationData loc = locationMgr.getLocation();
                Serial.printf("Location received: %s (%s, %.2f, %.2f)\n",
                              loc.city.c_str(), loc.timezone.c_str(), loc.latitude, loc.longitude);
                locationFetched = true;
            }
            else
            {
                Serial.println("Failed to get location, will use UTC0");
                locationFetched = false;
            }
            locationAttempted = true;
        }

        // 2. СИНХРОНИЗАЦИЯ NTP
        if (now - last_ntp_attempt > 10000UL)
        {
            bool needSync = false;
            String reason = "";

            if (locationAttempted && !firstSyncDone)
            {
                needSync = true;
                reason = "first sync";
            }
            else if (isRTCInvalid() && firstSyncDone)
            {
                needSync = true;
                reason = "RTC invalid";
            }
            else if (firstSyncDone && (millis() - lastNTPSync > NTP_SYNC_INTERVAL))
            {
                needSync = true;
                reason = "daily sync";
            }

            if (needSync)
            {
                last_ntp_attempt = now;

                LocationData loc = locationMgr.getLocation();

                String posixTz;
                if (locationFetched) {
                    posixTz = TimezoneConverter::toPosix(loc.timezone);
                    Serial.printf("Converting timezone: %s → %s\n",
                                  loc.timezone.c_str(), posixTz.c_str());
                } else {
                    posixTz = "UTC0";
                }

                Serial.printf("NTP sync (%s) with timezone: %s\n",
                              reason.c_str(), posixTz.c_str());

                DateTime dt_before = rtc.getDateTime();
                Serial.printf("Current RTC: %04d-%02d-%02d %02d:%02d:%02d\n",
                              dt_before.year, dt_before.month, dt_before.day,
                              dt_before.hour, dt_before.minute, dt_before.second);

                syncRTCWithNTP(rtc, posixTz);

                DateTime dt_after = rtc.getDateTime();

                time_t now_unix = time(nullptr);
                struct tm *utc_time   = gmtime(&now_unix);
                struct tm *local_time = localtime(&now_unix);

                Serial.printf("UTC time: %02d:%02d:%02d\n",
                              utc_time->tm_hour, utc_time->tm_min, utc_time->tm_sec);
                Serial.printf("Local time (system): %02d:%02d:%02d (offset: %d hours)\n",
                              local_time->tm_hour, local_time->tm_min, local_time->tm_sec,
                              local_time->tm_hour - utc_time->tm_hour);

                if (dt_after.year > 2023)
                {
                    Serial.printf("NTP sync successful! New RTC: %04d-%02d-%02d %02d:%02d:%02d\n",
                                  dt_after.year, dt_after.month, dt_after.day,
                                  dt_after.hour, dt_after.minute, dt_after.second);

                    lastNTPSync    = now;
                    ntpSynced      = true;
                    firstSyncDone  = true;
                }
                else
                {
                    Serial.println("NTP sync failed - RTC time invalid after sync");
                }
            }
        }

        // ====================== ОБНОВЛЕНИЕ ПОГОДЫ ======================
        static bool weather_requested_once = false;

        if (!weather_valid || (now - last_weather_ok > WEATHER_UPDATE_INTERVAL))
        {
            if (now - last_weather_try > 15000UL)
            {
                last_weather_try = now;

                Serial.println("[Weather] Attempting to fetch forecast...");

                if (fetchAndParseWeather())
                {
                    updateWeatherUI();                // обновляем экран погоды
                    updateMainScreenWeather();        // обновляем главный экран
                    weather_requested_once = true;
                }
                else if (weather_requested_once)
                {
                    lv_label_set_text(ui_weatherInfoToday, "Network error");
                    // также можно обновить главный экран индикацией ошибки
                }
            }
        }
    }
    else
    {
        // WiFi отключен — сбрасываем флаги
        if (locationAttempted || firstSyncDone)
        {
            Serial.println("WiFi disconnected, will re-sync on reconnection");
            locationAttempted = false;
            locationFetched   = false;
            firstSyncDone     = false;
            ntpSynced         = false;
        }

        // Если погода была показана, но Wi-Fi пропал — очищаем индикаторы
        if (weather_valid)
        {
            weather_valid = false;
            lv_label_set_text(ui_weatherInfoToday, "No connection");
            // можно также сбросить иконки на главном экране
        }
    }

    // ====================== ОБРАБОТКА ФИЗИЧЕСКОЙ КНОПКИ ======================
    btn.update();
    handleButtonPress();

    // ====================== LVGL TICK ======================
    if (now - last_tick >= 5)
    {
        lv_tick_inc(now - last_tick);
        last_tick = now;
    }
    lv_timer_handler();

    // ====================== ОБНОВЛЕНИЕ ДАТЧИКОВ (каждые 3 секунды) ======================
    if (now - last_sensor >= 3000)
    {
        UpdateSensorsAndTime();
        last_sensor = now;
    }

    // ====================== ОБНОВЛЕНИЕ WI-FI СТАТУСА (каждые 3 секунды) ======================
    if (now - last_wifi >= 3000)
    {
        UpdateWiFiStatus();
        last_wifi = now;
    }

    delay(1);
}
