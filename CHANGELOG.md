# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.0.2] - 2026-07-18

### Added
- Toolchain for ESP32 WROOM-32 boards, including a corresponding Dockerfile, CI build, and scripts to simplify building and flashing firmware on ESP32 WROOM-32 boards.
- Contribution guidelines and README updates explaining how to flash the embedded firmware to ESP32S3 and ESP32 WROOM-32 boards.
- Split and unsplit pages in the captive portal to prepare for Shamir's Secret Sharing code integration (in later releases).
- OpenOCD remote flash and monitor support for ESP32S3 boards (connected via a Raspberry Pi on the local network) to simplify flashing firmware on ESP32S3 boards.

### Removed
- Useless files remaining in the repository after the initial release.

## [0.0.1] - 2026-07-14

### Added
- Initial release of the Horcrux Box project with basic features and documentation (not an official working release).