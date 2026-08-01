# Horcrux Core

[![Zephyr](https://img.shields.io/badge/zephyr-v4.4.1-4B32C3?logo=zephyr)](https://www.zephyrproject.org/)
[![Build](https://github.com/ficaud/horcrux-core/actions/workflows/build.yml/badge.svg)](https://github.com/ficaud/horcrux-core/actions/workflows/build.yml)
[![Demo](https://img.shields.io/badge/demo-online-764ba2)](https://ficaud.github.io/horcrux-core/)

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

## How to connect to captive portal

Once ESP32 is loaded with the firmware, it will create a Wi-Fi access point called `Horcrux-XXXX` with the `X` bein g the last 4 characters of the ESP32 MAC address.

The access point is protected by the WPA2 password:
```
ubx7jrd9kje_ZRC8dqt
```

Connect to it and a captive portal will open automatically.

It is recommended to use another web browser to access the captive portal:
1. Tap on the cross icon in the top right corner of the portal to close it
2. Select "Use access point without internet" (or similar)
3. Open your favourite web browser and go to `http://192.168.4.1`

## Contribution

In [contribution](doc/contribution.md), you'll find all the required information to build and flash the Horcrux Core ESP32 firmware.