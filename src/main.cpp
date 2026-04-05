#include <Arduino.h>
#include <Adafruit_MAX1704X.h>
#include <BleGamepad.h>
#include <TouchHandler.h>

// project constants and pin definitions
#include "constants.h"

Adafruit_MAX17048 fuelGauge;

// Touch sensor debouncing - simple timer-based debounce
unsigned long lastForwardPressTime = 0;
unsigned long lastBackPressTime = 0;
bool forwardPressed = false;
bool backPressed = false;

unsigned long ledBlinkTimer = 0;

uint8_t lastBatteryPercent = 0;
unsigned long lastBatteryUpdate = 0;

BleGamepad bleGamepad(DEVICE_NAME, VENDOR_NAME);

const int touchPins[2] = {FORWARD_PIN, BACK_PIN};
TouchHandler touchHandler(touchPins, 2);

void setup()
{
    Serial.begin(SERIAL_MONITOR_SPEED);
    delay(1000); // give debugger time to open

    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(FORWARD_PIN, INPUT);
    pinMode(BACK_PIN, INPUT);

    touchHandler.begin();

    bleGamepad.begin();
    fuelGauge.begin();

    Serial.println("setup() completed");
}

void loop()
{
    unsigned long now = millis();
    if (bleGamepad.isConnected())
    {
        touchHandler.update();

        bool isForwardTouched = touchHandler.isTouched(0);
        bool isBackTouched = touchHandler.isTouched(1);

        // Uncomment to view TOUCH_THRESHOLD
        // Serial.print("Forward touched: ");
        // Serial.print(isForwardTouched);
        // Serial.print(" | ");
        // Serial.print(touchHandler.getThreshold(0));
        // Serial.print(" | Back touched: ");
        // Serial.print(isBackTouched);
        // Serial.print(" | ");
        // Serial.println(touchHandler.getThreshold(1));

        if (forwardPressed && !isForwardTouched)
        {
            bleGamepad.release(BUTTON_8); // right shoulder
            Serial.println("forward button released");
            forwardPressed = false;
        }
        if (!forwardPressed && isForwardTouched && now - lastForwardPressTime > TOUCH_DEBOUNCE_WINDOW)
        {
            bleGamepad.press(BUTTON_8);
            Serial.println("forward pressed -> page forward");
            lastForwardPressTime = now;
            forwardPressed = true;
        }

        if (backPressed && !isBackTouched)
        {
            bleGamepad.release(BUTTON_7); // left shoulder
            Serial.println("back button released");
            backPressed = false;
        }
        if (!backPressed && isBackTouched && now - lastBackPressTime > TOUCH_DEBOUNCE_WINDOW)
        {
            bleGamepad.press(BUTTON_7);
            Serial.println("back pressed -> page backward");
            lastBackPressTime = now;
            backPressed = true;
        }

        // every 30s
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

        // status LED: solid when connected
        digitalWrite(STATUS_LED_PIN, HIGH);
    }
    else
    {
        // If disconnected, blink LED
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
