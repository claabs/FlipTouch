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

bool isConnected = false;

// timer for battery updates
unsigned long lastBatteryUpdate = 0;

NimBLECharacteristic *pMidiCharacteristic = nullptr;
NimBLECharacteristic *pBatteryLevelCharacteristic = nullptr;

// Callback class for MIDI characteristics
struct MidiCallbacks : NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        uint8_t channel = pCharacteristic->getValue()[0];
        uint8_t note = pCharacteristic->getValue()[1];
        uint8_t velocity = pCharacteristic->getValue()[2];
        Serial.print("MIDI Note On: channel=");
        Serial.print(channel);
        Serial.print(", note=");
        Serial.print(note);
        Serial.print(", velocity=");
        Serial.println(velocity);
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

// Callback class for connection events
struct ConnectionCallbacks : NimBLEServerCallbacks
{
    bool isConnected = false;

    void onConnect(BLEServer *pServer, NimBLEConnInfo &connInfo) override
    {
        isConnected = true;
        Serial.println("Client connected (callback)");

        /**
         *  We can use the connection handle here to ask for different connection parameters.
         *  Args: connection handle, min connection interval, max connection interval
         *  latency, supervision timeout.
         *  Units; Min/Max Intervals: 1.25 millisecond increments.
         *  Latency: number of intervals allowed to skip.
         *  Timeout: 10 millisecond increments.
         */
        pServer->updateConnParams(connInfo.getConnHandle(), 24, 48, 0, 180);
    }

    void onDisconnect(BLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
    {
        isConnected = false;
        Serial.println("Client disconnected (callback)");
    }
};

void setup()
{
    Serial.begin(SERIAL_MONITOR_SPEED);
    delay(1000); // give debugger time to open

    // Register callbacks on characteristics using callback classes
    static MidiCallbacks midiCallbacks;
    static BatteryCallbacks batteryCallbacks;
    static ConnectionCallbacks connectionCallbacks;

    // initialise BLE
    NimBLEDevice::init(DEVICE_NAME);

    // Create BLE Server
    NimBLEServer *pServer = NimBLEDevice::createServer();

    pServer->setCallbacks(&connectionCallbacks);

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
    // pBatteryService->start();

    // Create MIDI Service (must be started before advertising)
    NimBLEService *pMidiService = pServer->createService(MIDI_SERVICE_UUID);

    pMidiCharacteristic = pMidiService->createCharacteristic(
        MIDI_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::NOTIFY |
            NIMBLE_PROPERTY::WRITE_NR);
    pMidiCharacteristic->setCallbacks(&midiCallbacks);

    // Add 2904 descriptor for UTF8 string formatting (special UUID handling)
    NimBLE2904 *pMidi2904 = pMidiCharacteristic->create2904();
    pMidi2904->setFormat(NimBLE2904::FORMAT_UTF8);

    pMidiService->start();

    // --- 4. Advertising ---
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(pMidiService->getUUID());
    // pAdvertising->addServiceUUID(pBatteryService->getUUID());
    // pAdvertising->setName(DEVICE_NAME);
    pAdvertising->setAppearance(0x41);      // MIDI Device
    pAdvertising->enableScanResponse(true); // Enable scan response for proper BLE behavior
    pAdvertising->start();

    NimBLEAddress mac = NimBLEDevice::getAddress();
    Serial.print("BLE MAC: ");
    Serial.print(mac.toString().c_str());
    Serial.println();

    Serial.println("setup() completed");
}

// Send MIDI note via BLE characteristic
void sendMidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity)
{
    if (isConnected)
    {
        uint8_t data[3] = {channel, note, velocity};
        pMidiCharacteristic->setValue(data, 3);
        pMidiCharacteristic->notify();
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
    if (isConnected)
    {
        uint8_t data[3] = {channel, note, velocity};
        pMidiCharacteristic->setValue(data, 3);
        pMidiCharacteristic->notify();
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
    { // only update when connected
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
