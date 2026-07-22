# SPDX-License-Identifier: Apache-2.0
#
# CMake build for the horcrux WASM demo.
#
# Requires Emscripten SDK.  Configure with:
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=/path/to/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
#
# Then build with:
#   cmake --build build

cmake_minimum_required(VERSION 3.20.0)
project(demo LANGUAGES C)

# ---------------------------------------------------------------------------
# Source paths
# ---------------------------------------------------------------------------
set(HORCRUX_CONNECT_SSS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src/sss)

# ---------------------------------------------------------------------------
# WASM flags
# ---------------------------------------------------------------------------
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -flto")

set(WASM_LINK_FLAGS
    -s MODULARIZE=1
    -s EXPORT_NAME=SSS
    -s EXPORTED_FUNCTIONS=_wasm_set_seed,_sss_split_wasm,_sss_combine_wasm,_malloc,_free
    -s EXPORTED_RUNTIME_METHODS=ccall,cwrap,getValue,setValue
    -s NO_EXIT_RUNTIME=1
    -s FILESYSTEM=0
    --no-entry
)

# ---------------------------------------------------------------------------
# Executable (WASM module)
# ---------------------------------------------------------------------------
# Output placed in scripts/ so the WASM-aware JS (split.js, unsplit.js)
# can load the module from the same directory.
add_executable(sss.wasm
    src/main.c
    src/sys_rand_stub.c
    ${HORCRUX_CONNECT_SSS_DIR}/sss.c        # reference parent project directly
)

set_target_properties(sss.wasm PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/scripts"
)

target_include_directories(sss.wasm PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src               # for demo_sss.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stubs          # for <zephyr/random/random.h>
    ${HORCRUX_CONNECT_SSS_DIR}                     # for sss.h (parent project reference)
)

target_link_options(sss.wasm PRIVATE ${WASM_LINK_FLAGS})
