

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

// BLE Service and Characteristic UUIDs
const char *BATTERY_SERVICE_UUID = "0000180F-0000-1000-8000-00805f9b34fb"; // Standard battery service
const char *BATTERY_LEVEL_UUID = "00002A19-0000-1000-8000-00805f9b34fb";   // Battery level characteristic

// MIDI Service UUIDs (custom)
const char *MIDI_SERVICE_UUID = "03b80e5a-ede8-4b33-a751-6ce34ec4c700"; // MIDI Note Service
const char *MIDI_CHARACTERISTIC_UUID = "7772e5db-3868-4112-a1a9-f2669d106bf3"; // MIDI Note Service
constexpr uint16_t MIDI_NOTE_ON_UUID = 0x2A17;      // MIDI Note On
constexpr uint16_t MIDI_NOTE_OFF_UUID = 0x2A18;     // MIDI Note Off
constexpr uint16_t MIDI_CHANNEL_UUID = 0x2A16;      // MIDI Channel

// Timing constants (milliseconds)
constexpr unsigned long BATTERY_UPDATE_INTERVAL = 60000;  // Update battery every 60s
constexpr unsigned long LED_BLINK_PERIOD = 500;           // LED blink period when disconnected

// MIDI message timing (milliseconds)
constexpr unsigned long MIDI_NOTE_DELAY = 50; // Delay between note on/off

// Small sleep to reduce CPU usage
constexpr unsigned long MAIN_LOOP_SLEEP = 10; // Main loop idle sleep

// MIDI Note parameters (channel, note number, velocity)
constexpr uint8_t MIDI_CHANNEL = 0;         // MIDI channel (0-15)
constexpr uint8_t FORWARD_NOTE_NUMBER = 60; // C4 middle C
constexpr uint8_t BACK_NOTE_NUMBER = 48;    // C3 lower octave
constexpr uint8_t NOTE_VELOCITY = 127;      // Maximum velocity

#endif // end header guard