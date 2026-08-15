# Contribution Guide

## Rules 

TBD

## Environment Setup

1. How to build and get into the .devcontainer

```bash
# Launch the dev container
devcontainer up --workspace-folder .

# Open a shell in the running dev container
devcontainer exec --workspace-folder . zsh
```

2. How to rebuild the devcontainer entirely

```bash
devcontainer up --workspace-folder . --remove-existing-container
```

3. How to exit the devcontainer

```bash
exit
```

## Running the Tests

The project uses Google Test for native unit tests (not on the ESP32). Tests are built and run with CMake presets.

### 1. Configure

```bash
cmake --preset tests
```

### 2. Build

```bash
cmake --build --preset tests
```

### 3. Run all tests (summary output)

```bash
ctest --test-dir build/tests
```

### 4. Run all tests with verbose output

Shows each test's command line, working directory and output in real time:

```bash
ctest --test-dir build/tests --verbose
```

### 5. Run a single test suite in isolation

Each suite is a standalone executable under `build/tests/tests/`:

```bash
# QR Code tests
./build/tests/tests/qrcode_test

# Shamir's Secret Sharing + QR tests
./build/tests/tests/sss_test

# QR → SVG conversion tests
./build/tests/tests/svg_test
```

### 6. Run a single test case with detailed output

```bash
# Run only one test case with full details (per-test timing, assertions)
./build/tests/tests/qrcode_test --gtest_filter='QRCodeTest.ShareGoldenGridMatches'

# Run only one test suite
./build/tests/tests/sss_test --gtest_filter='*'

# Multiple filters
./build/tests/tests/qrcode_test --gtest_filter='QRCodeTest.Encode*:QRCodeTest.Share*'
```

## Local WASM demo server

You can build the WASM demo server using the task: `Build Demo WASM (Makefile)`.

Then you can run the server on the devcontainer through the port forwaring feautre (8000):

```bash
cd /workspaces/horcrux/ && python3 demo/serve.py
```

Connect to it : `http://localhost:8000`
