# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

option(BUILD_TESTS "Build unit tests with Google Test" OFF)

# ─────────────────────────────────────────────────────────────────────────────
#  Dispatch: native unit tests  vs  Zephyr firmware
# ─────────────────────────────────────────────────────────────────────────────
if(BUILD_TESTS)

  project(horcrux-tests LANGUAGES C CXX)
  enable_testing()
  add_subdirectory(tests)

else()

  find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(wifi_apsta_mode)

  target_sources(app PRIVATE
      src/main.c
      src/access-point/http/src/http_server.c
      src/access-point/http/src/http_handlers.c
      src/access-point/http/src/http_types.c
      src/access-point/http/src/http_router.c
      src/access-point/net/src/dns.c
      src/access-point/net/src/dhcp.c
      src/access-point/wifi/src/wifi_mgr.c
      src/sss/sss.c
  )

  # Auto-generate page_captive.h from assets (HTML + CSS)
  set(ASSETS_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/src/access-point/assets)
  set(ASSETS_OUTPUT ${ASSETS_DIR}/page_captive.h)

  add_custom_command(
      OUTPUT  ${ASSETS_OUTPUT}
      COMMAND ${PYTHON_EXECUTABLE}
              ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed-assets.py
              ${ASSETS_DIR}
              ${ASSETS_OUTPUT}
      DEPENDS ${ASSETS_DIR}/index.html
              ${ASSETS_DIR}/split.html
              ${ASSETS_DIR}/unsplit.html
              ${ASSETS_DIR}/style.css
              ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed-assets.py
      COMMENT "Generating page_captive.h from assets..."
  )

  target_sources(app PRIVATE ${ASSETS_OUTPUT})

  target_include_directories(app PRIVATE
      src/access-point/http/inc
      src/access-point/net/inc
      src/access-point/wifi/inc
      src/access-point/assets
      src/sss
  )

endif()
