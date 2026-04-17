#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <WebServer.h>
#include "sensors/sensors.h"

#define AP_SSID             "MeteoNode-Setup"
#define AP_PASSWORD         "setup1234"       // можно сделать пустым ""
#define AP_CHANNEL          6
#define AP_TIMEOUT_SEC      600               // 10 мин, потом отключить AP

#define DNS_PORT            53
#define WEB_PORT            80

#define PREF_NS             "wifi"
#define PREF_SSID           "ssid"
#define PREF_PASS           "pass"

class WifiManager {
private:
    bool shouldRestart = false;
    unsigned long restartRequestedAt = 0;

    Sensors* sensorsPtr = nullptr;

    Preferences prefs;
    DNSServer dnsServer;
    WebServer server;

    bool apActive = false;
    unsigned long apStartTime = 0;

    String apIP = "192.168.4.1";

    void handleRoot();
    void handleSave();
    void handleNotFound();

public:
    void begin();
    void update();              // вызывать в loop()
    bool isConnected();
    String getIP();
    void resetWiFi();           // стереть настройки (для теста)

    void setSensors(Sensors* sensors) { sensorsPtr = sensors; }

    bool isInAPMode() const { return apActive; }
};

#endif