#include "wifi_manager.h"
#include "globals/global_values.h"
#include <ESPmDNS.h>

void WifiManager::begin() {
    prefs.begin(PREF_NS, false);
    String ssid = prefs.getString(PREF_SSID, "");
    String pass = prefs.getString(PREF_PASS, "");

    WiFi.mode(WIFI_AP_STA);
    delay(500);

    // Запускаем AP
    Serial.println("[WiFi] Starting AP without custom config...");
    
    if (WiFi.softAP(AP_SSID)) {
        Serial.printf("[WiFi] AP started! IP: %s\n", 
                      WiFi.softAPIP().toString().c_str());
        apActive = true;
        apStartTime = millis();
        
        // DNS: все домены → 192.168.4.1
        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
        Serial.println("[WiFi] Captive Portal DNS started");
    } else {
        Serial.println("[WiFi] FAILED to start AP");
        // Запасной вариант
        WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), 
                         IPAddress(255,255,255,0));
        if (WiFi.softAP(AP_SSID)) {
            Serial.printf("[WiFi] AP started on second try! IP: %s\n", 
                         WiFi.softAPIP().toString().c_str());
            apActive = true;
            apStartTime = millis();
            dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
            dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
            Serial.println("[WiFi] Captive Portal DNS started");
        }
    }

    // Подключаемся к домашнему WiFi, если есть сохранённые данные
    if (ssid.length() > 0) {
        Serial.printf("[WiFi] Attempting to connect to '%s' (password length: %d)\n", 
                      ssid.c_str(), pass.length());
        
        WiFi.begin(ssid.c_str(), pass.c_str());
        
        int cnt = 0;
        while (WiFi.status() != WL_CONNECTED && cnt < 20) {
            delay(500); 
            cnt++;
            Serial.print(".");
        }
        Serial.println();
        
        wl_status_t status = WiFi.status();
        if (status == WL_CONNECTED) {
            Serial.printf("[WiFi] ✓ Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        } else {
            Serial.printf("[WiFi] ✗ Failed to connect! Status: %d\n", status);
            switch(status) {
                case WL_NO_SSID_AVAIL: 
                    Serial.println("  → SSID not found"); 
                    break;
                case WL_CONNECT_FAILED: 
                    Serial.println("  → Bad password or weak signal"); 
                    break;
                case WL_DISCONNECTED: 
                    Serial.println("  → Disconnected"); 
                    break;
                default: 
                    Serial.printf("  → Error code: %d\n", status);
            }
        }
    } else {
        Serial.println("[WiFi] No saved credentials — AP mode only");
    }

    // Роуты веб-сервера
    server.on("/", [this]() { handleRoot(); });
    server.on("/save", HTTP_POST, [this]() { handleSave(); });

    // ==================== НАСТРОЙКИ ====================
    server.on("/get_thresholds", HTTP_GET, [this]() {
        String json = "{"
            "\"max_temp\":" + String(thresholds.max_temp, 1) + 
            ",\"min_temp\":" + String(thresholds.min_temp, 1) + 
            ",\"max_hum\":" + String(thresholds.max_hum, 1) + 
            ",\"min_hum\":" + String(thresholds.min_hum, 1) + 
            ",\"max_co2\":" + String(thresholds.max_co2) + 
            ",\"day_b\":" + String(brightness.day_brightness) + 
            ",\"night_b\":" + String(brightness.night_brightness) + 
            ",\"auto_b\":" + String(brightness.auto_brightness ? "true" : "false") + 
            "}";
        server.send(200, "application/json", json);
    });

    server.on("/set_thresholds", HTTP_POST, [this]() {
        Serial.println("[WiFi] === set_thresholds called ===");
    
        if (server.hasArg("max_temp")) {
            String val = server.arg("max_temp");
            val.replace(',', '.');
            thresholds.max_temp = val.toFloat();
        }
        if (server.hasArg("min_temp")) {
            String val = server.arg("min_temp");
            val.replace(',', '.');
            thresholds.min_temp = val.toFloat();
        }
        if (server.hasArg("max_hum")) {
            String val = server.arg("max_hum");
            val.replace(',', '.');
            thresholds.max_hum = val.toFloat();
        }
        if (server.hasArg("min_hum")) {
            String val = server.arg("min_hum");
            val.replace(',', '.');
            thresholds.min_hum = val.toFloat();
        }
        if (server.hasArg("max_co2")) {
            thresholds.max_co2 = server.arg("max_co2").toInt();
        }
        
        if (thresholds.max_temp > 100 || thresholds.max_temp < -50) thresholds.max_temp = 32.0f;
        if (thresholds.max_hum > 100 || thresholds.max_hum < 0)   thresholds.max_hum = 75.0f;
        
        if (server.hasArg("day_b"))   brightness.day_brightness   = server.arg("day_b").toInt();
        if (server.hasArg("night_b")) brightness.night_brightness = server.arg("night_b").toInt();
        if (server.hasArg("auto_b"))  brightness.auto_brightness  = 
            (server.arg("auto_b") == "true" || server.arg("auto_b") == "1");

        saveThresholds();
        saveBrightness();
        
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // ==================== РАСПИСАНИЕ ====================
    server.on("/set_schedule", HTTP_POST, [this]() {
        for (int i = 0; i < 7; i++) {
            String prefix = "d" + String(i) + "_";
            if (server.hasArg(prefix + "start")) {
                String s = server.arg(prefix + "start");
                int colon = s.indexOf(':');
                if (colon > 0) {
                    brightnessSchedule.days[i].startHour = s.substring(0, colon).toInt();
                    brightnessSchedule.days[i].startMin  = s.substring(colon + 1).toInt();
                }
            }
            if (server.hasArg(prefix + "end")) {
                String s = server.arg(prefix + "end");
                int colon = s.indexOf(':');
                if (colon > 0) {
                    brightnessSchedule.days[i].endHour = s.substring(0, colon).toInt();
                    brightnessSchedule.days[i].endMin  = s.substring(colon + 1).toInt();
                }
            }
        }
        if (server.hasArg("use_schedule")) {
            brightnessSchedule.useSchedule = (server.arg("use_schedule") == "1");
        }
        saveSchedule();
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/get_schedule", HTTP_GET, [this]() {
        String json = "{\"use_schedule\":" + String(brightnessSchedule.useSchedule ? "true" : "false");
        for (int i = 0; i < 7; i++) {
            json += ",\"d" + String(i) + "_start\":\"" + 
                    String(brightnessSchedule.days[i].startHour) + ":" + 
                    (brightnessSchedule.days[i].startMin < 10 ? "0" : "") + 
                    String(brightnessSchedule.days[i].startMin) + "\"";
            json += ",\"d" + String(i) + "_end\":\"" + 
                    String(brightnessSchedule.days[i].endHour) + ":" + 
                    (brightnessSchedule.days[i].endMin < 10 ? "0" : "") + 
                    String(brightnessSchedule.days[i].endMin) + "\"";
        }
        json += "}";
        server.send(200, "application/json", json);
    });

    // ==================== ДАННЫЕ ====================
    server.on("/data", HTTP_GET, [this]() {
        if (!sensorsPtr) { 
            server.send(500, "text/plain", "No sensors"); 
            return; 
        }
        SensorData d = sensorsPtr->read();
        String json = "{\"temperature\":" + String(d.temperature) + 
                      ",\"humidity\":" + String(d.humidity) + 
                      ",\"pressure\":" + String(d.pressure) + 
                      ",\"aqi\":" + String(d.aqi) + 
                      ",\"tvoc\":" + String(d.tvoc) + 
                      ",\"co2\":" + String(d.co2) + "}";
        server.send(200, "application/json", json);
    });

    server.on("/history", HTTP_GET, [this]() {
        if (!sensorsPtr) { 
            server.send(500, "text/plain", "No sensors"); 
            return; 
        }
        
        Preferences prefs;
        bool exists = prefs.begin("history", true);
        
        if (!exists) {
            server.send(200, "application/json", "[]");
            return;
        }
        
        String json = "[";
        int count = prefs.getInt("count", 0);
        int start = max(0, count - 1440);
        
        bool first = true;
        for (int i = start; i < count; i++) {
            String key = "h" + String(i);
            if (prefs.isKey(key.c_str())) {
                if (!first) json += ",";
                first = false;
                json += prefs.getString(key.c_str(), "{}");
            }
        }
        json += "]";
        prefs.end();
        
        server.send(200, "application/json", json);
    });

    server.on("/ping", HTTP_GET, [this]() {
        server.send(200, "text/plain", "pong");
    });

    server.onNotFound([this]() { handleNotFound(); });
    server.begin();

    Serial.println("[WiFi] WebServer started on 192.168.4.1");
}

// ==================== ОСТАЛЬНЫЕ МЕТОДЫ ====================
void WifiManager::update() {
    static bool printedIp = false;
    
    if (WiFi.status() == WL_CONNECTED && !printedIp) {
        Serial.printf("[WiFi] Connected! Local IP: %s\n", WiFi.localIP().toString().c_str());
        printedIp = true;
    }
    if (WiFi.status() != WL_CONNECTED) {
        printedIp = false;
    }

    // DNS только если AP активен
    if (apActive) {
        dnsServer.processNextRequest();
        
        // Выключаем AP через 30 минут после подключения к домашнему WiFi
        if (WiFi.status() == WL_CONNECTED && millis() - apStartTime > AP_TIMEOUT_SEC * 1000UL) {
            Serial.println("[WiFi] AP timeout → отключаем точку доступа");
            WiFi.softAPdisconnect(true);
            apActive = false;
        }
    }
    
    // mDNS при подключении к WiFi
    if (WiFi.status() == WL_CONNECTED && !mdnsStarted) {
        if (MDNS.begin("meteonode")) {
            MDNS.addService("http", "tcp", 80);
            Serial.println("[WiFi] mDNS started: meteonode.local");
            mdnsStarted = true;
        }
    }
    if (WiFi.status() != WL_CONNECTED) {
        mdnsStarted = false;
    }
    
    server.handleClient();
    
    if (shouldRestart && millis() - restartRequestedAt > 4000) {
        ESP.restart();
    }
}

bool WifiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WifiManager::getIP() {
    if (apActive) return WiFi.softAPIP().toString();
    if (isConnected()) return WiFi.localIP().toString();
    return "нет";
}

void WifiManager::resetWiFi() {
    prefs.clear();
    Serial.println("[WiFi] Settings cleared → restarting");
    ESP.restart();
}

// ==================== СТРАНИЦЫ ====================

void WifiManager::handleRoot() {
    String html = R"raw(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MeteoNode — Настройка Wi-Fi</title>
    <style>
        body {font-family:Arial,sans-serif;background:#667eea;color:white;margin:0;padding:0;min-height:100vh;display:flex;align-items:center;justify-content:center;}
        .container {background:rgba(255,255,255,0.95);color:#333;padding:40px;border-radius:16px;box-shadow:0 10px 30px rgba(0,0,0,0.3);max-width:420px;width:90%;text-align:center;}
        h1 {margin:0 0 24px;font-size:28px;color:#1a237e;}
        p {margin:0 0 32px;font-size:16px;color:#555;}
        input {width:100%;padding:14px;margin:12px 0;border:1px solid #ddd;border-radius:8px;font-size:16px;box-sizing:border-box;}
        button {width:100%;padding:16px;background:#3f51b5;color:white;border:none;border-radius:8px;font-size:18px;cursor:pointer;transition:background 0.3s;}
        button:hover {background:#303f9f;}
        .footer {margin-top:24px;font-size:14px;color:#777;}
    </style>
</head>
<body>
    <div class="container">
        <h1>MeteoNode</h1>
        <p>Настройте подключение к Wi-Fi</p>
        <form action="/save" method="POST">
            <input type="text" name="ssid" placeholder="Имя сети (SSID)" required autofocus>
            <input type="password" name="pass" placeholder="Пароль" required>
            <button type="submit">Подключиться</button>
        </form>
        <div class="footer">После сохранения устройство перезагрузится</div>
    </div>
</body>
</html>
)raw";
    server.send(200, "text/html; charset=utf-8", html);
}

void WifiManager::handleSave() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");

    ssid.trim();
    pass.trim();
    
    Serial.printf("[WiFi] Save called — SSID: '%s', PASS length: %d\n", 
                  ssid.c_str(), pass.length());

    if (ssid.length() == 0) {
        String errPage = R"raw(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <title>Ошибка</title>
</head>
<body style="font-family:Arial;text-align:center;padding:80px;color:#c62828;">
    <h2>Ошибка</h2>
    <p>SSID не может быть пустым</p>
    <a href="/">Назад</a>
</body>
</html>
)raw";
        server.send(200, "text/html; charset=utf-8", errPage);
        return;
    }

    prefs.putString(PREF_SSID, ssid);
    prefs.putString(PREF_PASS, pass);
    
    String saved_ssid = prefs.getString(PREF_SSID, "");
    Serial.printf("[WiFi] Verified saved SSID: '%s'\n", saved_ssid.c_str());

    String okPage = R"raw(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Успех</title>
    <style>body{font-family:Arial;text-align:center;padding:60px;background:#e8f5e9;color:#1b5e20;}.icon{font-size:80px;margin:30px 0;}h1{margin:0 0 20px;}</style>
</head>
<body>
    <div class="icon">✅</div>
    <h1>Настройки сохранены!</h1>
    <p>Wi-Fi: <b>SSSS</b></p>
    <p>Устройство перезагрузится через 4 секунды.</p>
    <p>Можно закрыть вкладку.</p>
    <script>setTimeout(function(){window.close();}, 4000);</script>
</body>
</html>
)raw";

    okPage.replace("SSSS", ssid);
    server.send(200, "text/html; charset=utf-8", okPage);

    shouldRestart = true;
    restartRequestedAt = millis();
}

void WifiManager::handleNotFound() {
    String path = server.uri();
    
    // ===== CAPTIVE PORTAL CHECKS =====
    // Эти URL проверяются устройствами для обнаружения Captive Portal
    if (path == "/generate_204" ||          // Android
        path == "/gen_204" ||               // Старый Android
        path == "/hotspot-detect.html" ||    // Windows
        path == "/library/test/success.html" || // iOS
        path == "/success.txt" ||            // iOS
        path == "/canonical.html" ||         // Firefox
        path == "/ncsi.txt" ||               // Windows NCSI
        path == "/connecttest.txt" ||        // Windows 10+
        path == "/redirect" ||               // Общий
        path == "/fwlink/") {                // Microsoft
        
        // Отдаём успешный ответ, но с редиректом на страницу настройки
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/plain", "");
        return;
    }
    
    // ===== API-ЗАПРОСЫ НЕ РЕДИРЕКТИМ =====
    if (path.startsWith("/data") || 
        path.startsWith("/save") || 
        path.startsWith("/ping") ||
        path.startsWith("/get_") ||
        path.startsWith("/set_") ||
        path.startsWith("/history") ||
        path.startsWith("/debug")) {
        server.send(404, "text/plain", "Not found");
        return;
    }
    
    // ===== ВСЁ ОСТАЛЬНОЕ → РЕДИРЕКТ НА ГЛАВНУЮ =====
    Serial.printf("[Captive] Redirecting '%s' to /\n", path.c_str());
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "Redirecting...");
}