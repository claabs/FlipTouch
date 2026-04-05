# FlipTouch

FlipTouch is a capacitive touch input Bluetooth page turner for music reading apps.

* **MIDI BLE** - sends page turn commands over HID gamepad protocol to avoid on-screen keyboard conflicts with HID keyboard style page turners.
* **Two capacitive touch pads** – one for forward (right shoulder) and one for back (left shoulder).
* **Single‑colour status LED** – flashes when unpaired, stays solid when connected.
* **Battery monitoring via an Adafruit MAX1704x LiPo fuel gauge** – percentage is reported over the BLE battery level characteristic.
* **External reset switch** on the RESET pin is used for power‑down; firmware does not have to manage sleep.

## Operation

Forward pad press sends a right shoulder press; back pad sends a left shoulder press.  No special BLE pairing button is required – the controller connects to any host device that requests it. According to the BLE gamepad library, [iOS is not supported](https://github.com/lemmingDev/ESP32-BLE-Gamepad?tab=readme-ov-file#features).

Battery percentage is continuously updated to the central device; many music apps display this automatically once the keyboard is connected.

## Hardware

* Adafruit ESP32‑S3 Feather with 4 MB Flash / 2 MB PSRAM
* Rechargeable LiPo battery
* Power toggle switch from RESET to GND (manual shutdown)
* Single‑colour LED
* Two capacitive/touch pads wired to D5 and D6
* Copper tape, wires, project box, etc.

## Wiring

* **Forward touch pad** → D5 (GPIO5)
* **Back touch pad** → D6 (GPIO6)
* **Status LED** → D9 (GPIO9) – drive it high for on, use a suitable resistor.
* **LiPo battery** connected to the Feather's battery connector as normal.

## LED Behavior

* Flashing when BLE status is unpaired (500 ms on/off)
* Solid when BLE status is actively paired

