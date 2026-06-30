#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <WebServer.h>
#include "sensors/sensors.h"

// Вернули макросы на место
#define AP_SSID             "MeteoNode-Setup"
#define AP_TIMEOUT_SEC      1800
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
    bool mdnsStarted = false;
    
    void handleRoot();
    void handleSave();
    void handleNotFound();

public:
    void begin();
    void update();
    bool isConnected();
    String getIP();
    void resetWiFi();
    void setSensors(Sensors* sensors) { sensorsPtr = sensors; }
    bool isInAPMode() const { return apActive; }
};

#endif