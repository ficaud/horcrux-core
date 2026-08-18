# Contribution Guide

## Rules

TBD

## Environment Setup

The project ships a dev container that provides a ready-to-use build
environment. See `doc/commands.md` for the full list of devcontainer
commands.

1. **Build and enter the dev container**

   ```bash
   devcontainer up --workspace-folder .
   devcontainer exec --workspace-folder . zsh
   ```

2. **Rebuild the dev container from scratch**

   ```bash
   devcontainer up --workspace-folder . --remove-existing-container
   ```

3. **Exit the dev container**

   ```bash
   exit
   ```

## Running the Tests

The project uses Google Test for native unit tests (they run on the host, not
the ESP32). Tests are built and run via CMake presets.

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

Shows each test's command line, working directory, and output in real time:

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
# Run one test case with full details (per-test timing, assertions)
./build/tests/tests/qrcode_test --gtest_filter='QRCodeTest.ShareGoldenGridMatches'

# Run every test in a suite
./build/tests/tests/sss_test --gtest_filter='*'

# Run several cases matching multiple filters
./build/tests/tests/qrcode_test --gtest_filter='QRCodeTest.Encode*:QRCodeTest.Share*'
```

## Local WASM Demo Server

Build the WASM demo using the `Build Demo WASM (Makefile)` task, then start a
local server. In the dev container, use port forwarding on port 8000:

```bash
cd /workspaces/relic-core/ && python3 demo/serve.py
```

Then open `http://localhost:8000`.
