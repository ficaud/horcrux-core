# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.0.4] - 2026-07-18

### Added
- `sss`: Shamir's Secret Sharing algorithm implementation for splitting and reconstructing secrets using GF(2^8) finite fields.
- `unit_tests/sss`: Unit tests (using Google Test) with deterministic test vectors, as well as cross-validation with the [dsprenkels/sss](https://github.com/dsprenkels/sss) project to ensure correctness of the implementation.
- Version number display in the captive portal pages to indicate the current version of the Horcrux Box project (#4).
- Split / unsplit pages: Management of Shamir's Secret Sharing split and reconstruct operations from the captive portal pages using plain text and copy helpers.

### Changed
- `embed-assets`: The script now takes separate JavaScript files, minifies them, and embeds them into the captive portal pages (to ease web page maintenance and readability).

### Fixed
- Devcontainer: User permissions are now set correctly to avoid getting stuck when requiring root permissions to run commands in the devcontainer.
- `http_server`: Increased stack and query size limits to avoid errors on large secrets to split or reconstruct, such as Bitcoin seed phrases (#12).

## [0.0.3] - 2026-07-18

### Added
- Toolchain for ESP32 WROOM-32 boards, including a corresponding Dockerfile, CI build, and scripts to simplify building and flashing firmware on ESP32 WROOM-32 boards.
- Contribution guidelines and README updates explaining how to flash the embedded firmware to ESP32S3 and ESP32 WROOM-32 boards.
- Split and unsplit pages in the captive portal to prepare for Shamir's Secret Sharing code integration (in later releases).
- OpenOCD remote flash and monitor support for ESP32S3 boards (connected via a Raspberry Pi on the local network) to simplify flashing firmware on ESP32S3 boards.

### Removed
- Useless files remaining in the repository after the initial release.

## [0.0.2] - 2026-07-14

### Added
- Initial release of the Horcrux Box project with basic features and documentation (not an official working release).