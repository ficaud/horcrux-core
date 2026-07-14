# Horcrux ESP32

## WARNING This is a work in progress. The hardware and software are not yet ready for production use.

## Quick Start - programming

### Via ESPWEBTOOL


### Via ESPTOOL



## Quick Start - contribution

### Build and Flash

In the development environment, you can build and program the Horcrux ESP32 using `tasks.json` in VSCode:

```bash
Ctrl + Shift + P → Tasks: Run Task → Pristine Build (esp32s3_devkitc/procpu) or Build (esp32s3_devkitc/procpu)
```

Then, from the host where the ESP32 is connected, flash the firmware as follows:

```bash
./host-flash.sh
```

### Monitor the Serial Output

**Note:** You need `west` installed in your host environment to monitor the serial output. Install it using pip:

```bash
pip install west
```

From the host where the ESP32 is connected, monitor the serial output as follows:

```bash
source ~/zephyrproject/.venv/bin/activate
west espressif monitor
```