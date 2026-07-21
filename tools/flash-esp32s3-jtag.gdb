# GDB script to flash ESP32-S3 via remote OpenOCD (Raspberry Pi)
#
# Usage:
#   source .env
#   xtensa-...-gdb -x tools/flash-esp32s3-jtag.gdb \
#     -ex "target remote ${RPI_IP_ADDRESS}:3333" \
#     build/zephyr/zephyr.elf
#
# The wrapper .elf (zephyr.bin wrapped in an ELF) must already exist.
# RPI_IP_ADDRESS is defined in ../.env

mon reset halt
load build/zephyr/zephyr-wrapper.elf
mon reset
detach
quit
