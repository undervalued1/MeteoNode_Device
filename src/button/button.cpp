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
    Serial.println("Button initialized on GPIO21");
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

            if (stableState == LOW) {           // нажали
                pressStartTime = now;
                currentState = BUTTON_PRESSED;
            } 
            else {                              // отпустили
                unsigned long duration = now - pressStartTime;

                if (duration >= longPressTime) {
                    currentState = BUTTON_LONG_PRESSED;
                } 
                else if (duration > 40) {       // защита от дребезга
                    clickCount++;
                    lastClickTime = now;
                    currentState = BUTTON_CLICKED;
                }
            }
        }
    }

    // Двойной клик
    if (clickCount > 0 && (now - lastClickTime) > doubleClickTime) {
        clickCount = 0;
    }

    lastReading = reading;
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
        currentState = BUTTON_IDLE;
        return true;
    }
    return false;
}