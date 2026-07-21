# Horcrux Core

## WARNING This is a work in progress. The hardware and software are not yet ready for production use.

A device that uses a local, easy-to-use Shamir's Secret Sharing scheme to generate or assemble sensitive information.

Features:
- **Local operation**: Everything is done disconnected from the internet.
- **Easy to use**: Captive portal to generate and assemble secrets.
- **Longevity**: No dependency on external services.
- **Open source**: Fully auditable.

## Quick Start - flash ESP32

What boards ?
* [ESP32-S3-DevKitC-1](https://docs.zephyrproject.org/latest/boards/espressif/esp32s3_devkitc/doc/index.html)
* [ESP32-DevKit-V1](https://docs.zephyrproject.org/latest/boards/others/doit_esp32_devkit_v1/doc/index.html)

Where is the latest firmware release ?
* [last release](https://github.com/ficaud/horcrux-core/releases)


### Via ESPWEBTOOL

1. Go to [ESPWEBTOOL](https://esptool.spacehuhn.com/) and connect to your board by selecting the correct serial port.

2. Reset the board in bootloader mode:
   - Press and hold the **BOOT** button.
   - While holding **BOOT**, press and release the **EN/RESET** button.
   - Release the **BOOT** button.

3. In the memory map, remove all sectors except the one containing the firmware:

   | Board | Keep this sector | Flash offset |
   |---|---|---|
   | **ESP32-S3** | `0x00000` (first sector) | `0x0` |
   | **ESP32** | `0x01000` (second sector) | `0x1000` |

   > The ESP32 ROM bootloader requires the firmware at offset `0x1000` (the first 4 KB sector is reserved). The ESP32-S3 accepts it at `0x0`.

4. Select the firmware binary file and click **PROGRAM**.

### Via ESPTOOL (Python)

**Requirements** :

- Python 3.6+
- [esptool](https://github.com/espressif/esptool) Python package

**1. Install esptool**

```bash
pip install esptool
```

Verify the installation:

```bash
python3 -c "import esptool; print(esptool.__version__)"
```

**2. Connect the board and identify the serial port**

| OS | Typical port |
|---|---|
| **macOS** | `/dev/cu.usbmodem*` or `/dev/cu.usbserial-*` |
| **Linux** | `/dev/ttyACM0` or `/dev/ttyUSB0` |
| **Windows** | `COM1`, `COM2`, etc. |

List available ports:

```bash
# macOS
ls /dev/cu.usbmodem* /dev/cu.usbserial* 2>/dev/null

# Linux
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null

# Windows (PowerShell)
python -m esptool chip_id
```

**3. Put the board in bootloader (download) mode**

1. Press and hold the **BOOT** button.
2. While holding **BOOT**, press and release the **EN/RESET** button.
3. Release the **BOOT** button.

The board is now ready to receive firmware.

**4. Flash the firmware**

Erase the flash first (recommended for a clean slate):

> ⚠️ Activate your Python virtual environment first if you use one (especially on macOS).

| Board | Erase command |
|---|---|
| **ESP32-S3** | `esptool --chip esp32s3 --port <PORT> erase_flash` |
| **ESP32** | `esptool --chip esp32 --port <PORT> erase_flash` |

Then write the firmware — **the chip type, baud rate, and flash offset differ** between boards:

**ESP32-S3** (flash offset `0x0`):
```bash
esptool --chip esp32s3 --port <PORT> --baud 921600 \
  --before default-reset --after hard-reset \
  write-flash 0x0 path/to/firmware.bin
```

**ESP32** (flash offset `0x1000`):
```bash
esptool --chip esp32 --port <PORT> --baud 460800 \
  --before default-reset --after hard-reset \
  write-flash 0x1000 path/to/firmware.bin
```

Replace `<PORT>` with the port from step 2, and `path/to/firmware.bin` with the compiled binary (e.g., `build/zephyr/zephyr.bin`).

> Why `0x1000` for ESP32? The first 4 KB flash sector (`0x0000`–`0x0FFF`) is reserved by the ESP32 ROM bootloader for metadata. The ESP32-S3 ROM does not have this requirement, so the image starts at `0x0`.

## Quick Start - contribution

Below are the basic steps to build, flash, and monitor the serial output of the Horcrux Core firmware on the ESP32.

## Contribution

In [contribution](doc/contribution.md), you'll find all the required information to build and flash the Horcrux Core ESP32 firmware.