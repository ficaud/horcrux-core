# SPDX-License-Identifier: Apache-2.0

# Allow this file to be used both as a standalone project
# (cmake -S tests -B tests/build) and as a subdirectory of the
# root dispatcher (cmake -S . -B build -DBUILD_TESTS=ON).
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.20.0)
    project(horcrux-tests LANGUAGES C CXX)
    enable_testing()
endif()

# ---------------------------------------------------------------------------
# Google Test (from system packages)
# ---------------------------------------------------------------------------
find_package(GTest REQUIRED)

# ---------------------------------------------------------------------------
# sss_ref — dsprenkels/sss (hazmat API) for cross-validation
# ---------------------------------------------------------------------------
set(SSS_REF_DIR         ${CMAKE_CURRENT_SOURCE_DIR}/../external/sss)
set(SSS_RANDOMBYTES_DIR ${SSS_REF_DIR}/randombytes)

# ---------------------------------------------------------------------------
# Source paths
# ---------------------------------------------------------------------------
set(SSS_SRC_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/../src/sss)
set(QRCODE_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src/qrcode)
set(SVG_SRC_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/../src/svg)

# ---------------------------------------------------------------------------
# sss_test — Shamir + QR Code tests (combined executable)
# ---------------------------------------------------------------------------
add_executable(sss_test
    main.cpp
    test_sss.cpp
    test_qrcode.cpp
    mockup/sys_rand_stub.c                   # Linux stub for Zephyr's sys_rand_get()
    ${SSS_SRC_DIR}/sss.c                      # Our Shamir implementation
    ${QRCODE_SRC_DIR}/qr_encode.c             # QR Code generator
    ${SSS_REF_DIR}/hazmat.c                      # sss_ref low-level SSS
    ${SSS_RANDOMBYTES_DIR}/randombytes.c         # dsprenkels/randombytes
)

target_include_directories(sss_test PRIVATE
    ${SSS_SRC_DIR}                               # for shamir.h
    ${QRCODE_SRC_DIR}                            # for qr_encode.h
    ${SSS_REF_DIR}                               # for hazmat.h
    ${SSS_RANDOMBYTES_DIR}                       # for randombytes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs            # for <zephyr/random/random.h>
)

target_link_libraries(sss_test PRIVATE
    GTest::GTest
    GTest::Main
)

# ---------------------------------------------------------------------------
# qrcode_test — QR Code tests only (standalone executable)
# Build with: cmake --build tests/build-native --target qrcode_test
# ---------------------------------------------------------------------------
add_executable(qrcode_test
    main.cpp
    test_qrcode.cpp
    ${QRCODE_SRC_DIR}/qr_encode.c             # QR Code generator
)

target_include_directories(qrcode_test PRIVATE
    ${QRCODE_SRC_DIR}                            # for qr_encode.h
)

target_link_libraries(qrcode_test PRIVATE
    GTest::GTest
    GTest::Main
)

# ---------------------------------------------------------------------------
# svg_test — QR Code → SVG conversion tests only (standalone executable)
# Build with: cmake --build tests/build-native --target svg_test
# ---------------------------------------------------------------------------
add_executable(svg_test
    main.cpp
    test_svg.cpp
    ${QRCODE_SRC_DIR}/qr_encode.c             # QR Code generator
    ${SVG_SRC_DIR}/qrcode_to_svg.c            # QR Code → SVG converter
)

target_include_directories(svg_test PRIVATE
    ${QRCODE_SRC_DIR}                            # for qr_encode.h
    ${SVG_SRC_DIR}                               # for qrcode_to_svg.h
)

# Absolute path to the golden reference SVG directory so the tests can locate
# the seed-phrase share reference files regardless of the working directory.
target_compile_definitions(svg_test PRIVATE
    REFERENCE_SVG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/reference/svg"
)

target_link_libraries(svg_test PRIVATE
    GTest::GTest
    GTest::Main
)

# ---------------------------------------------------------------------------
# Register tests with ctest
# ---------------------------------------------------------------------------
add_test(NAME sss_test COMMAND sss_test)

# Launch the QR code unit tests (QRCodeTest suite) via the dedicated binary.
add_test(NAME qrcode_test COMMAND qrcode_test)

# Launch the SVG unit tests (SVGTest suite) via the dedicated binary.
add_test(NAME svg_test COMMAND svg_test)
