#include "button.h"

Button::Button() 
    : lastDebounceTime(0)
    , pressStartTime(0)
    , lastReading(HIGH)
    , stableState(HIGH)
    , clickCount(0)
    , lastClickTime(0)
    , currentState(BUTTON_IDLE) 
{
}

void Button::begin() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Serial.printf("Button initialized on GPIO%d\n", BUTTON_PIN);
    lastReading = digitalRead(BUTTON_PIN);
    stableState = lastReading;
}

void Button::update() {
    unsigned long now = millis();
    bool reading = digitalRead(BUTTON_PIN);
    
    if (reading != lastReading) {
        lastDebounceTime = now;
    }
    
    if ((now - lastDebounceTime) > debounceDelay) {
        if (reading != stableState) {
            stableState = reading;
            
            if (stableState == LOW) {
                pressStartTime = now;
                currentState = BUTTON_PRESSED;
            } else {
                if (now - pressStartTime >= longPressTime) {
                    currentState = BUTTON_LONG_PRESSED;
                } else {
                    clickCount++;
                    lastClickTime = now;
                    currentState = BUTTON_CLICKED;
                }
            }
        }
    }
    
    if (clickCount > 0 && (now - lastClickTime) > doubleClickTime) {
        clickCount = 0;
    }
    
    lastReading = reading;
}

bool Button::isPressed() {
    return (stableState == LOW);
}

bool Button::isLongPressed() {
    if (currentState == BUTTON_LONG_PRESSED) {
        currentState = BUTTON_IDLE;
        return true;
    }
    return false;
}

bool Button::isClicked() {
    if (currentState == BUTTON_CLICKED) {
        currentState = BUTTON_IDLE;
        return true;
    }
    return false;
}

bool Button::isDoubleClicked() {
    if (clickCount >= 2) {
        clickCount = 0;
        return true;
    }
    return false;
}

ButtonState Button::getState() {
    ButtonState state = currentState;
    if (state != BUTTON_PRESSED) {
        currentState = BUTTON_IDLE;
    }
    return state;
}

void Button::reset() {
    clickCount = 0;
    currentState = BUTTON_IDLE;
}