#include <Arduino.h>
#include <Bounce2.h>
#include <Wire.h>
#include <Adafruit_MAX1704X.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEService.h>
#include <NimBLECharacteristic.h>
#include <NimBLEAdvertising.h>

// project constants and pin definitions
#include "constants.h"


// physical objects
Bounce forwardBtn = Bounce();
Bounce backBtn = Bounce();
Adafruit_MAX17048 fuelGauge;

// flag recorded after setup runs
bool gaugePresent = false;

// timer used for simple LED blink when not paired
unsigned long ledBlinkTimer = 0;

// helper that stores last reported battery percent so we don't spam BLE
uint8_t lastBatteryPercent = 0;

// connection management
unsigned long connectionCheckTimer = 0;

bool isConnected = false;

// timer for battery updates
unsigned long lastBatteryUpdate = 0;


NimBLECharacteristic *pMidiNoteOnCharacteristic = nullptr;
NimBLECharacteristic *pMidiNoteOffCharacteristic = nullptr;
NimBLECharacteristic *pMidiChannelCharacteristic = nullptr;
NimBLECharacteristic *pBatteryLevelCharacteristic = nullptr;

// Callback class for MIDI characteristics
struct MidiCallbacks : NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        uint8_t channel = pCharacteristic->getValue()[0];
        uint8_t note = pCharacteristic->getValue()[1];

        if (pCharacteristic == pMidiNoteOnCharacteristic)
        {
            uint8_t velocity = pCharacteristic->getValue()[2];
            Serial.print("MIDI Note On: channel=");
            Serial.print(channel);
            Serial.print(", note=");
            Serial.print(note);
            Serial.print(", velocity=");
            Serial.println(velocity);
        }
        else if (pCharacteristic == pMidiNoteOffCharacteristic)
        {
            Serial.print("MIDI Note Off: channel=");
            Serial.print(channel);
            uint8_t note = pCharacteristic->getValue()[1];
            Serial.print(", note=");
            Serial.println(note);
        }
        else if (pCharacteristic == pMidiChannelCharacteristic)
        {
            Serial.print("MIDI Channel: ");
            Serial.println(channel);
        }
    }
};

// Callback class for battery characteristic
struct BatteryCallbacks : NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        uint8_t initialLevel = 100;
        pCharacteristic->setValue(&initialLevel, 1);
    }

    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        Serial.println("Battery level write attempted");
    }
};

void setup()
{
    Serial.begin(SERIAL_MONITOR_SPEED);
    delay(100); // give debugger time to open

    // initialise BLE
    NimBLEDevice::init(DEVICE_NAME);

    // Create BLE Server
    NimBLEServer *pServer = NimBLEDevice::createServer();

    // Register callbacks on characteristics using callback classes
    static MidiCallbacks midiCallbacks;
    static BatteryCallbacks batteryCallbacks;

    // Create Battery Service
    NimBLEService *pBatteryService = pServer->createService(BATTERY_SERVICE_UUID);
    pBatteryLevelCharacteristic = pBatteryService->createCharacteristic(
        BATTERY_LEVEL_UUID,
        NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::NOTIFY);

    // Set initial value (e.g., 100%)
    uint8_t initialLevel = 100;
    pBatteryLevelCharacteristic->setValue(&initialLevel, 1);

    // Register battery callbacks
    pBatteryLevelCharacteristic->setCallbacks(&batteryCallbacks);

    // Start battery service BEFORE advertising so it's available to clients
    pBatteryService->start();

    // Create MIDI Service (must be started before advertising)
    NimBLEService *pMidiService = pServer->createService(MIDI_NOTE_SERVICE_UUID);

    // Create MIDI Note On characteristic
    pMidiNoteOnCharacteristic = pMidiService->createCharacteristic(
        MIDI_NOTE_ON_UUID,
        NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::WRITE_NR);
    pMidiNoteOnCharacteristic->setCallbacks(&midiCallbacks);

    // Create MIDI Note Off characteristic
    pMidiNoteOffCharacteristic = pMidiService->createCharacteristic(
        MIDI_NOTE_OFF_UUID,
        NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::WRITE_NR);
    pMidiNoteOffCharacteristic->setCallbacks(&midiCallbacks);

    // Create MIDI Channel characteristic
    pMidiChannelCharacteristic = pMidiService->createCharacteristic(
        MIDI_CHANNEL_UUID,
        NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::WRITE_NR);
    pMidiChannelCharacteristic->setCallbacks(&midiCallbacks);

    pMidiService->start();

    // --- 3. Advertising ---
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(MIDI_NOTE_SERVICE_UUID);
    pAdvertising->addServiceUUID(BATTERY_SERVICE_UUID);
    pAdvertising->start();

    // Initialize battery update timer
    lastBatteryUpdate = millis();

    Serial.println("setup() completed");
}

// Send MIDI note via BLE characteristic
void sendMidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity)
{
    if (pMidiNoteOnCharacteristic && isConnected)
    {
        uint8_t data[3] = {channel, note, velocity};
        pMidiNoteOnCharacteristic->setValue(data, 3);
        pMidiNoteOnCharacteristic->notify();
        Serial.print("Sent MIDI Note On: ch=");
        Serial.print(channel);
        Serial.print(", note=");
        Serial.println(note);
    }
    else if (!isConnected)
    {
        Serial.println("Cannot send MIDI: no connection");
    }
}

void sendMidiNoteOff(uint8_t channel, uint8_t note, uint8_t velocity)
{
    if (pMidiNoteOffCharacteristic && isConnected)
    {
        uint8_t data[3] = {channel, note, velocity};
        pMidiNoteOffCharacteristic->setValue(data, 3);
        pMidiNoteOffCharacteristic->notify();
        Serial.print("Sent MIDI Note Off: ch=");
        Serial.print(channel);
        Serial.println(", note=");
    }
    else if (!isConnected)
    {
        Serial.println("Cannot send MIDI: no connection");
    }
}

// Helper to send a complete MIDI note event (on + off)
void sendMidiNote(uint8_t channel, uint8_t note, uint8_t velocity)
{
    sendMidiNoteOn(channel, note, velocity);
    delay(MIDI_NOTE_DELAY);
    sendMidiNoteOff(channel, note, velocity);
}

void loop()
{
    // update debounced buttons
    forwardBtn.update();
    backBtn.update();

    // Check connection state periodically (new NimBLE API)
    if (millis() - connectionCheckTimer > CONNECTION_CHECK_INTERVAL)
    {
        connectionCheckTimer = millis();
        
        NimBLEServer *pServer = NimBLEDevice::getServer();
        if (pServer && pServer->getConnectedCount() > 0)
        {
            isConnected = true;
            Serial.println("Client connected");
        }
        else
        {
            isConnected = false;
            Serial.println("No client connected");
        }
    }

    // forward pad pressed (falling edge)
    if (forwardBtn.fell())
    {
        sendMidiNote(MIDI_CHANNEL, FORWARD_NOTE_NUMBER, NOTE_VELOCITY);
        Serial.println("forward pressed -> page forward");
    }

    // back pad pressed
    if (backBtn.fell())
    {
        sendMidiNote(MIDI_CHANNEL, BACK_NOTE_NUMBER, NOTE_VELOCITY);
        Serial.println("back pressed -> page backward");
    }

    // periodically sample battery and push level to host (adaptive interval)
    if (gaugePresent && isConnected)
    {                                         // only update when connected
        unsigned long t = millis();
        if ((t - lastBatteryUpdate) > BATTERY_UPDATE_INTERVAL) // every 60s
        {
            float perc = fuelGauge.cellPercent(); // returns a float percentage
            uint8_t soc = (uint8_t)perc;          // truncate to integer for BLE
            if (soc != lastBatteryPercent)
            {
                pBatteryLevelCharacteristic->setValue(&soc, 1);
                pBatteryLevelCharacteristic->notify();
                lastBatteryPercent = soc;
                Serial.print("Battery %=");
                Serial.println(soc);
            }
            lastBatteryUpdate = t;
        }
    }

    // status LED: solid when connected, blink when not
    if (isConnected)
    {
        digitalWrite(STATUS_LED_PIN, HIGH);
    }
    else
    {
        unsigned long t = millis();
        if (((t / LED_BLINK_PERIOD) % 2) == 0)
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
