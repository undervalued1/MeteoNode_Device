#include "sensors.h"

Sensors::Sensors() : ahtOk(false), ensOk(false), bmpOk(false), lastReadTime(0) {
    lastData.isValid = false;
    lastData.temperature = -100;
    lastData.humidity = -100;
    lastData.pressure = -100;
    lastData.altitude = 0;
    lastData.aqi = -1;
    lastData.co2 = -1;
    lastData.tvoc = -1;
}

bool Sensors::begin() {
    Serial.println("\n=== Initializing Sensors ===");
    
    // Инициализация I2C
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);  // 100 кГц (стандарт)
// или даже Wire.setClock(50000); для 50 кГц
    Serial.printf("I2C initialized on SDA:GPIO%d, SCL:GPIO%d\n", I2C_SDA, I2C_SCL);
    
    // AHT20
    Serial.print("AHT20... ");
    ahtOk = aht20.begin();
    Serial.println(ahtOk ? "OK" : "FAIL");
    
    // ENS160 — используем адрес 0x53 (большинство комбо-модулей)
    Serial.print("ENS160 (ScioSense lib, addr 0x53)... ");
    ensOk = ens160.begin(false);           // true = включить отладочный вывод
    if (!ensOk) {
    Serial.println("Trying alternative address 0x52...");
    ens160 = ScioSense_ENS160(ENS160_I2CADDR_0);  // переинициализируем на 0x52
    ensOk = ens160.begin(true);
}
    if (ensOk) {
        delay(100);
        
        // Показываем версию прошивки (очень полезно для диагностики)
        Serial.printf("FW v%d.%d.%d   ", ens160.getMajorRev(), ens160.getMinorRev(), ens160.getBuild());
        
        // Проверяем, отвечает ли датчик вообще
        if (ens160.getMajorRev() == 0 && ens160.getMinorRev() == 0) {
            Serial.println("← версия 0.0.0 — возможно плохой контакт или чип не отвечает");
        } else {
            Serial.println("OK");
        }
        
        // Переводим в стандартный режим
        ens160.setMode(ENS160_OPMODE_STD);
        delay(50);
    } else {
        Serial.println("begin() failed");
    }
    
    // BMP180
    Serial.print("BMP180... ");
    bmpOk = bmp180.begin();
    Serial.println(bmpOk ? "OK" : "FAIL");
    
    Serial.println("=== Initialization Complete ===\n");
    return (ahtOk || ensOk || bmpOk);
}

void Sensors::readAHT20(SensorData &data) {
    if (ahtOk) {
        if (aht20.available()) {
            data.temperature = aht20.getTemperature();
            data.humidity    = aht20.getHumidity();
        } else {
            data.temperature = lastData.temperature;
            data.humidity    = lastData.humidity;
        }
    }
}

void Sensors::readENS160(SensorData &data) {
    if (ensOk) {
        // Передаём компенсацию температуры и влажности (очень важно для точности!)
        if (data.temperature > -40 && data.humidity > 0) {
            ens160.set_envdata(data.temperature, data.humidity);
        }

        // Делаем измерение (ждём новых данных)
        bool newData = ens160.measure(true);   // true = ждать новых данных

        if (newData) {
            data.aqi = ens160.getAQI();        // 1..5
            data.co2 = ens160.geteCO2();       // eCO2 в ppm
            data.tvoc = ens160.getTVOC();      // TVOC в ppb
        } else {
            // Если новых данных нет — оставляем старые значения
            data.aqi = lastData.aqi;
            data.co2 = lastData.co2;
            data.tvoc = lastData.tvoc;
        }
    }
}

void Sensors::readBMP180(SensorData &data) {
    if (bmpOk) {
        char status;
        double T, P;

        status = bmp180.startTemperature();
        if (status) {
            delay(status);
            bmp180.getTemperature(T);

            status = bmp180.startPressure(3);
            if (status) {
                delay(status);
                bmp180.getPressure(P, T);
                data.pressure = P;  // уже в hPa
                
                // Рассчитываем высоту сразу при получении давления
                data.altitude = calculateAltitude(data.pressure);
            }
        }
    } else {
        data.pressure = lastData.pressure;
        data.altitude = lastData.altitude;  // сохраняем предыдущее значение
    }
}

float Sensors::calculateAltitude(float pressure, float seaLevelPressure)
{
    if (pressure <= 0) return 0;
    return 44330.0 * (1.0 - pow(pressure / seaLevelPressure, 1.0/5.255));
}

SensorData Sensors::read() {
    SensorData data = lastData;
    
    readAHT20(data);
    readENS160(data);
    readBMP180(data);
    
    data.isValid = (data.temperature > -50 && data.temperature < 100);
    
    lastData = data;
    lastReadTime = millis();
    
    return data;
}

void Sensors::printToSerial(SensorData data) {
    Serial.println("\n┌─────────────────────────┐");
    Serial.println("│     SENSOR DATA         │");
    Serial.println("├─────────────────────────┤");
    
    if (ahtOk) {
        Serial.printf("│ Temperature: %6.1f °C    │\n", data.temperature);
        Serial.printf("│ Humidity:    %6.0f %%     │\n", data.humidity);
    } else {
        Serial.println("│ AHT20:       NOT FOUND  │");
    }
    
    if (ensOk) {
        Serial.printf("│ AQI:         %6d (%s) │\n", data.aqi, getAQIString(data.aqi).c_str());
        Serial.printf("│ eCO2:        %6d ppm   │\n", data.co2);
        Serial.printf("│ TVOC:        %6d ppb   │\n", data.tvoc);
    } else {
        Serial.println("│ ENS160:      NOT FOUND  │");
    }
    
    if (bmpOk) {
        Serial.printf("│ Pressure:    %6.0f hPa   │\n", data.pressure);
        Serial.printf("│ Pressure:    %6.0f m   │\n", data.altitude);
    } else {
        Serial.println("│ BMP180:      NOT FOUND  │");
    }
    
    Serial.println("└─────────────────────────┘\n");
}

String Sensors::getAQIString(int aqi) {
    switch(aqi) {
        case 1: return "Excellent";
        case 2: return "Good";
        case 3: return "Moderate";
        case 4: return "Poor";
        case 5: return "Unhealthy";
        default: return "Unknown";
    }
}