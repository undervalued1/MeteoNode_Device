#ifndef LOCATION_MANAGER_H
#define LOCATION_MANAGER_H

#include <Arduino.h>

struct LocationData
{
    float latitude;
    float longitude;
    String timezone;
    String city;          // добавлено поле для названия города
    bool valid;
};

class LocationManager
{
public:
    LocationManager();
    bool begin();
    bool update();
    LocationData getLocation();

private:
    LocationData location;
};

#endif