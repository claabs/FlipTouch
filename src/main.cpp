#include <Arduino.h>
#include <Adafruit_MAX1704X.h>
#include <BleGamepad.h>

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

void setup()
{
    Serial.begin(SERIAL_MONITOR_SPEED);
    delay(1000); // give debugger time to open

    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(FORWARD_PIN, INPUT);
    pinMode(BACK_PIN, INPUT);

    bleGamepad.begin();
    fuelGauge.begin();

    Serial.println("setup() completed");
}

void loop()
{
    unsigned long now = millis();
    if (bleGamepad.isConnected())
    {
        touch_value_t forwardValue = touchRead(FORWARD_PIN); // must read to trigger interrupt
        touch_value_t backValue = touchRead(BACK_PIN);

        // Uncomment to calibrate TOUCH_THRESHOLD
        // Serial.print("Forward touch value: ");
        // Serial.print(forwardValue);
        // Serial.print(" | Back touch value: ");
        // Serial.println(backValue);

        if (forwardPressed && forwardValue <= TOUCH_THRESHOLD)
        {
            bleGamepad.release(DPAD_RIGHT);
            Serial.println("forward button released");
            forwardPressed = false;
        }
        if (!forwardPressed && forwardValue > TOUCH_THRESHOLD && now - lastForwardPressTime > TOUCH_DEBOUNCE_WINDOW)
        {
            bleGamepad.press(DPAD_RIGHT);
            Serial.println("forward pressed -> page forward");
            lastForwardPressTime = now;
            forwardPressed = true;
        }

        if (backPressed && backValue <= TOUCH_THRESHOLD)
        {
            bleGamepad.release(DPAD_LEFT);
            Serial.println("back button released");
            backPressed = false;
        }
        if (!backPressed && backValue > TOUCH_THRESHOLD && now - lastBackPressTime > TOUCH_DEBOUNCE_WINDOW)
        {
            bleGamepad.press(DPAD_LEFT);
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
