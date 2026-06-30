#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include <ScioSense_ENS160.h>           // ← новая библиотека
#include <SparkFun_Qwiic_Humidity_AHT20.h>
#include <SFE_BMP180.h>

// Пины I2C для WT32-SC01 Plus
#define I2C_SDA 10
#define I2C_SCL 11

struct SensorData {
    float temperature;     // °C
    float humidity;        // % RH
    float pressure;        // hPa
    float altitude;        // m
    int aqi;               // 1-5
    int co2;               // ppm (eCO2)
    int tvoc;              // ppb
    bool isValid;
};

class Sensors {
private:
    ScioSense_ENS160 ens160 = ScioSense_ENS160(ENS160_I2CADDR_1);               
    AHT20 aht20;
    SFE_BMP180 bmp180;
    
    bool ahtOk;
    bool ensOk;
    bool bmpOk;
    
    SensorData lastData;
    unsigned long lastReadTime;
    
    void readAHT20(SensorData &data);
    void readENS160(SensorData &data);
    void readBMP180(SensorData &data);
    
public:
    Sensors();
    bool begin();
    SensorData read();
    void printToSerial(SensorData data);
    String getAQIString(int aqi);
    bool isAHT20_ok() { return ahtOk; }
    bool isENS160_ok() { return ensOk; }
    bool isBMP180_ok() { return bmpOk; }
    float calculateAltitude(float pressure, float seaLevelPressure = 1013.25);
};

#endif