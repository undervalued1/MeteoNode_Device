#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

#define BUTTON_PIN 21

enum ButtonState {
    BUTTON_IDLE,
    BUTTON_PRESSED,
    BUTTON_LONG_PRESSED,
    BUTTON_CLICKED
};

class Button {
private:
    unsigned long lastDebounceTime;
    unsigned long pressStartTime;
    bool lastReading;
    bool stableState;
    int clickCount;
    unsigned long lastClickTime;
    
    const unsigned long debounceDelay = 50;
    const unsigned long longPressTime = 2000;   
    const unsigned long doubleClickTime = 400;
    
    ButtonState currentState;
    
public:
    Button();
    void begin();
    void update();
    bool isPressed();
    bool isLongPressed();
    bool isClicked();
    bool isDoubleClicked();
    ButtonState getState();
    void reset();
};

#endif