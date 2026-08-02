# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.2.2] - 2026-08-02

### Fixed
 - `dns.c`: allow a list of domain names to trigger the captive portal serving (this is to have an automated portal generating on connection)

## [1.2.1] - 2026-08-02

### Added
- `.clangd`: Added a clangd configuration file to use clangd as the C/C++ language server instead of the VSCode IntelliSense engine, providing more accurate parsing and indexing of the project.
- `logo`: Added a new logo to the project in the readme and the flash page.
- `test_qrcode.cpp`: Added new unit tests for the QR code generation and parsing ([#17](https://github.com/ficaud/horcrux-core/issues/17)).
- `test_svg.cpp`: Added new unit tests for the SVG generation from QR codes 
([#17](https://github.com/ficaud/horcrux-core/issues/17)).

### Changed
- `devcontainer`: Update the devcontainer to install nvim, clangd, lazyvim, lazygit and ohmyzsh to ease the development experience.
- `dns.c`: Only accept url "horcrux.co" to serve the captive portal ([#29](https://github.com/ficaud/horcrux-core/issues/29)).

### Fixed
- `release.yml`: Removed the `.elf` and `.map` files from the release artifacts and updated the release README with the new instructions to flash the ESP32.
- `qrcode_to_svg`, `qr_encode`: Minor fixes to pass the unit tests.

## [1.2.0] - 2026-08-01

### Added
- `split.html`: QR code creation and SVG rendering have been added to the split page to allow users to easily share secrets with others by scanning the QR code ([#19](https://github.com/ficaud/horcrux-core/issues/19)).
- GPLv3 license has been added to the project to ensure that it remains free and open-source for all users ([#24](https://github.com/ficaud/horcrux-core/issues/24)).

### Fixed
- `flash.html`: Replaced the back button with a back-to-demo page button (for the WASM demo).
- `devcontainer`: Removed `clangd`, which was causing conflicts with IntelliSense in VSCode and was not needed for the project.
- `c_cpp_properties.json`: Fixed the compiler path and the overall configuration to ensure that the project is properly parsed and indexed by the C/C++ extension in VSCode.
- `http_handler`: Moved some local buffers to static to reduce stack usage and avoid stack overflow when sending large secrets to split or reconstruct.

### Removed
- `contribution`: Left the contribution documentation empty for now, as it is not yet ready to be published. It will be added in a future release.
- `page_captive.h`: Removed the auto-generated captive portal header for ESP32 firmware, as it does not need to be versioned by Git ([#26](https://github.com/ficaud/horcrux-core/issues/26)).

## [1.1.1] - 2026-07-27

### Fixed
- `unplsit.html`: Qr code scanning was not working properly on either WASM demo or on the ESP32 captive portal (#23, #19).
- `index.html`: typo

## [1.1.0] - 2026-07-26

### Added
- `demo/flash.html`: A new web page has been added to the demo that allows users to flash the Horcrux Core firmware to their ESP32 boards directly from the browser using the Web Serial API.
- `unsplit.html`: A camera button has been added to allow users to scan a QR code to retrieve the secret from the Horcrux Core device instead of typing it manually (#18).
- `badges`: Zephyr version badge, build status badge, and demo link badge have been added to the README.md file to provide quick access to relevant information about the project.

### Changed
- `wifi_mgr`: The Wi-Fi MAC address is now used to customize the access point name (SSID) to avoid conflicts when multiple devices are in the same area (#16).
- `release`: Artifacts are now renamed to clearly indicate which platform they are built for (ESP32-S3-DevKitC-1 or ESP32-DevKit-V1).

### Fixed
- `http_server`: Increased the maximum size of the HTTP request body to avoid errors when sending large secrets to split or reconstruct, such as Bitcoin seed phrases.

### Removed
- `workflow`: Removed the automatic build and pages generation workflow when pushing to the `dev/jfi` branch.

## [1.0.0] - 2026-07-22

First version of the Horcrux Core project that provides the basics of what it is intended to do: split and reconstruct secrets using Shamir's Secret Sharing (SSS) over GF(256) on an embedded device, with a captive portal to manage the operations.

### Added
- `demo/wasm`: A new demo page that runs Shamir's Secret Sharing (SSS) entirely in the browser using WebAssembly (WASM) has been built and deployed to GitHub Pages. The demo page allows users to get an overview of the captive portal that will be displayed on the embedded device (#10).

### Changed
- `horcrux-core`: The old project was renamed to `horcrux-core` and now includes only the embedded firmware with the demo WASM page.

### Removed
- `readme`: ESPWebTool is no longer used as a means to flash devices because it is unstable, and it's difficult to know if it is still maintained or even working at any given time.

## [0.0.4] - 2026-07-18

### Added
- `sss`: Shamir's Secret Sharing algorithm implementation for splitting and reconstructing secrets using GF(2^8) finite fields.
- `unit_tests/sss`: Unit tests (using Google Test) with deterministic test vectors, as well as cross-validation with the [dsprenkels/sss](https://github.com/dsprenkels/sss) project to ensure correctness of the implementation.
- Version number display in the captive portal pages to indicate the current version of the Horcrux Core project (#4).
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
- Initial release of the Horcrux Core project with basic features and documentation (not an official working release).
