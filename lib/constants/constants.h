

#ifndef MY_CONSTANTS_H // header guard
#define MY_CONSTANTS_H

#include <Arduino.h>

//******************************************************
//? Constants
//*******************************************************/

constexpr int FORWARD_PIN = T5;            // forward touch pad input (active low)
constexpr int BACK_PIN = T6;               // back touch pad input (active low)
constexpr int STATUS_LED_PIN = GPIO_NUM_9; // single‑colour status LED (active high)

constexpr int SERIAL_MONITOR_SPEED = 115200;

const char *DEVICE_NAME = "FlipTouch"; // BLE device name
const char *VENDOR_NAME = "claabs";    // BLE vendor name

// *******************************************************
//   Other constants
// *******************************************************


// Timing constants (milliseconds)
constexpr unsigned long BATTERY_UPDATE_INTERVAL = 60000;  // Update battery every 60s
constexpr unsigned long LED_BLINK_PERIOD = 500;           // LED blink period when disconnected

// HID message timing (milliseconds)
constexpr unsigned long BUTTON_PRESS_LENGTH = 50; // Delay between note on/off
constexpr unsigned long TOUCH_DEBOUNCE_WINDOW = 100; // ms between button presses
constexpr uint32_t  TOUCH_THRESHOLD = 50; // Touch threshold for ESP32 touch pins


// Small sleep to reduce CPU usage
constexpr unsigned long MAIN_LOOP_SLEEP = 10; // Main loop idle sleep

#endif // end header guard