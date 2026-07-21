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
set(SSS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src/sss)

# ---------------------------------------------------------------------------
# Test executable
# ---------------------------------------------------------------------------
add_executable(sss_test
    main.cpp
    test_sss.cpp
    mockup/sys_rand_stub.c                   # Linux stub for Zephyr's sys_rand_get()
    ${SSS_SRC_DIR}/sss.c                      # Our Shamir implementation
    ${SSS_REF_DIR}/hazmat.c                      # sss_ref low-level SSS
    ${SSS_RANDOMBYTES_DIR}/randombytes.c         # dsprenkels/randombytes
)

target_include_directories(sss_test PRIVATE
    ${SSS_SRC_DIR}                               # for shamir.h
    ${SSS_REF_DIR}                               # for hazmat.h
    ${SSS_RANDOMBYTES_DIR}                       # for randombytes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs            # for <zephyr/random/random.h>
)

target_link_libraries(sss_test PRIVATE
    GTest::GTest
    GTest::Main
)

# ---------------------------------------------------------------------------
# Register test with ctest
# ---------------------------------------------------------------------------
add_test(NAME sss_test COMMAND sss_test)
