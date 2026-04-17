#include "wifi_manager.h"

void WifiManager::begin() {
    prefs.begin(PREF_NS, false);

    String ssid = prefs.getString(PREF_SSID, "");
    String pass = prefs.getString(PREF_PASS, "");

    Serial.printf("[WiFi begin] Saved SSID: '%s'\n", ssid.c_str());

    bool connected = false;

    if (ssid.length() > 0) {
        Serial.printf("[WiFi] Connecting to %s...\n", ssid.c_str());

        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pass.c_str());

        int cnt = 0;
        while (WiFi.status() != WL_CONNECTED && cnt < 30) {
            delay(500);
            Serial.print(".");
            cnt++;
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            Serial.print("[WiFi] Connected → IP: ");
            Serial.println(WiFi.localIP());
            connected = true;
        } else {
            Serial.println("[WiFi] Connection failed");
        }
    } else {
        Serial.println("[WiFi] No saved credentials");
    }

    // 🔥 ВСЕГДА запускаем AP (и при успехе, и при провале)
    WiFi.mode(WIFI_AP_STA);

    WiFi.softAPConfig(
        IPAddress(192,168,4,1),
        IPAddress(192,168,4,1),
        IPAddress(255,255,255,0)
    );

    WiFi.softAP(AP_SSID, ""); // без пароля для теста

    Serial.printf("[WiFi] AP started: %s at 192.168.4.1\n", AP_SSID);

    apActive = true;
    apStartTime = millis();

    // DNS (для captive portal)
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    dnsServer.setTTL(300);
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);

    Serial.println("[WiFi] DNS server started");

    // Web сервер
    server.on("/", [this]() {
        Serial.println("HTTP request → /");
        handleRoot();
    });

    server.on("/save", HTTP_POST, [this]() {
        Serial.println("HTTP POST → /save");
        handleSave();
    });

    server.onNotFound([this]() {
        Serial.println("HTTP 404 → redirect");
        handleNotFound();
    });

    
    server.on("/data", HTTP_GET, [this]() {
        Serial.println("HTTP GET → /data");
    
    if (sensorsPtr == nullptr) {
        server.send(500, "application/json", "{\"error\":\"sensors not initialized\"}");
        return;
    }
    
    SensorData data = sensorsPtr->read();  // Читаем реальные данные!
    
    String json = "{";
    json += "\"temperature\":" + String(data.temperature) + ",";
    json += "\"humidity\":" + String(data.humidity) + ",";
    json += "\"pressure\":" + String(data.pressure) + ",";
    json += "\"aqi\":" + String(data.aqi) + ",";
    json += "\"tvoc\":" + String(data.tvoc) + ",";
    json += "\"co2\":" + String(data.co2);
    json += "}";
    
    server.send(200, "application/json", json);
    Serial.println("Sent: " + json);
});

    server.begin();
    Serial.println("[WiFi] Web server started");

    if (connected) {
        Serial.println("[WiFi] Running in AP + STA mode");
    } else {
        Serial.println("[WiFi] Running in AP ONLY mode");
    }
}

void WifiManager::update() {
    if (apActive) {
        dnsServer.processNextRequest();
        server.handleClient();

        static uint32_t lastLog = 0;
        if (millis() - lastLog > 10000) {
            Serial.println("AP active → waiting for client requests...");
            lastLog = millis();
        }

        if (millis() - apStartTime > AP_TIMEOUT_SEC * 1000UL) {
            Serial.println("[WiFi] AP timeout → disconnect");
            WiFi.softAPdisconnect(true);
            apActive = false;
        }
    }

    if (shouldRestart && millis() - restartRequestedAt > 4000) {
        Serial.println("[WiFi] Restarting after save...");
        shouldRestart = false;
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
    Serial.println("handleNotFound: redirecting to http://192.168.4.1");
    server.sendHeader("Location", "http://192.168.4.1", true);
    server.send(302, "text/plain", "Redirecting to config page...");
}