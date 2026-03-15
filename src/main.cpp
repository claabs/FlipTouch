#include <Arduino.h>
#include <Adafruit_MAX1704X.h>
#include <BleGamepad.h>

// project constants and pin definitions
#include "constants.h"

Adafruit_MAX17048 fuelGauge;

// Touch sensor debouncing - simple timer-based debounce
unsigned long lastForwardPressTime = 0;
unsigned long lastBackPressTime = 0;

unsigned long ledBlinkTimer = 0;

uint8_t lastBatteryPercent = 0;
unsigned long lastBatteryUpdate = 0;

BleGamepad bleGamepad(DEVICE_NAME, VENDOR_NAME);

// Helper to send a complete button event (on + off)
void sendButtonPress(uint8_t button)
{
    bleGamepad.press(button);
    delay(BUTTON_PRESS_LENGTH);
    bleGamepad.release(button);
}

void forwardTouch()
{
    // Debounce: only trigger if enough time has passed since last press
    unsigned long now = millis();
    if ((now - lastForwardPressTime) > TOUCH_DEBOUNCE_WINDOW)
    {
        sendButtonPress(DPAD_RIGHT);
        Serial.println("forward pressed -> page forward");
        lastForwardPressTime = now;
    }
}

void backTouch()
{
    // Debounce: only trigger if enough time has passed since last press
    unsigned long now = millis();
    if ((now - lastBackPressTime) > TOUCH_DEBOUNCE_WINDOW)
    {
        sendButtonPress(DPAD_LEFT);
        Serial.println("back pressed -> page backward");
        lastBackPressTime = now;
    }
}

void setup()
{
    Serial.begin(SERIAL_MONITOR_SPEED);
    delay(1000); // give debugger time to open

    bleGamepad.begin();

    touchAttachInterrupt(FORWARD_PIN, forwardTouch, TOUCH_THRESHOLD);
    touchAttachInterrupt(BACK_PIN, backTouch, TOUCH_THRESHOLD);

    pinMode(STATUS_LED_PIN, OUTPUT);

    fuelGauge.begin();

    Serial.println("setup() completed");
}

void loop()
{
    if (bleGamepad.isConnected())
    {

        // every 60s
        unsigned long now = millis();
        if ((now - lastBatteryUpdate) > BATTERY_UPDATE_INTERVAL)
        {
            float perc = fuelGauge.cellPercent(); // returns a float percentage
            if (isnan(perc))
            {
                Serial.println("battery gauge not found");
            }
            else
            {
                uint8_t soc = (uint8_t)perc; // truncate to integer for BLE
                if (soc != lastBatteryPercent)
                {
                    bleGamepad.setBatteryLevel(soc);
                    bleGamepad.setBatteryPowerInformation(POWER_STATE_GOOD);

                    lastBatteryPercent = soc;
                    Serial.print("Battery %=");
                    Serial.println(soc);
                }
                lastBatteryUpdate = now;
            }
        }

        // status LED: solid when connected, blink when not
        digitalWrite(STATUS_LED_PIN, HIGH);
    }
    else
    {
        unsigned long now = millis();
        if (((now / LED_BLINK_PERIOD) % 2) == 0)
        {
            digitalWrite(STATUS_LED_PIN, HIGH);
        }
        else
        {
            digitalWrite(STATUS_LED_PIN, LOW);
        }
    }

    delay(MAIN_LOOP_SLEEP); // small sleep to reduce CPU usage
}
