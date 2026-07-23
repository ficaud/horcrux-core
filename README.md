# Horcrux Core

Easy to use, offline, open source, and secure secret sharing for your digital life.

Horcrux Core is a firmware for ESP32 boards that implements Shamir's Secret Sharing algorithm to split and recover secrets (like passwords, private keys, bitcoin seed phrases etc.) in a secure way.

See the [demo](https://ficaud.github.io/horcrux-core/) in a WASM type web page hosted by github to see how it works.

## Boards compatibility

This project's firmware is compatible with the following ESP32 boards:
* [ESP32-S3-DevKitC-1](https://docs.zephyrproject.org/latest/boards/espressif/esp32s3_devkitc/doc/index.html)
* [ESP32-DevKit-V1](https://docs.zephyrproject.org/latest/boards/others/doit_esp32_devkit_v1/doc/index.html)


## How to flash the firmware

1. Open the **[web flasher](https://ficaud.github.io/horcrux-core/flash.html)** in Chrome or Edge.
2. Connect your ESP32 board via USB.
3. Put the board in **download mode**:
   - Hold **BOOT**, tap **RESET**, release **BOOT**.
4. Click **Connect & Flash**, select the serial port when prompted.
5. Wait for the progress bar to complete — done!

## Contribution

In [contribution](doc/contribution.md), you'll find all the required information to build and flash the Horcrux Core ESP32 firmware.