# SPDX-License-Identifier: Apache-2.0

# Devcontainer for Zephyr RTOS development on ESP32-S3-DevKitC-1
# Includes: Zephyr SDK (xtensa-esp32s3 toolchain), west, esptool, clang-format

FROM ubuntu:24.04

LABEL description="Zephyr RTOS dev environment for ESP32-S3-DevKitC-1"

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# ------------------------------------------------------------------
# 1. System dependencies
# ------------------------------------------------------------------
RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    ninja-build \
    gperf \
    ccache \
    dfu-util \
    device-tree-compiler \
    make \
    file \
    xz-utils \
    python3 \
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-tk \
    python3-wheel \
    python3-venv \
    gcc \
    g++ \
    git \
    wget \
    curl \
    lsb-release \
    sudo \
    unzip \
    doxygen \
    clang-format \
    libsdl2-dev \
    libmagic1 \
    lz4 \
    lzop \
    zstd \
    net-tools \
    screen \
    socat \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# ------------------------------------------------------------------
# 2. Python packages
# ------------------------------------------------------------------
RUN pip3 install --no-cache-dir --break-system-packages \
    west \
    pyelftools \
    esptool \
    anytree \
    pykwalify \
    python-magic \
    psutil \
    pyyaml \
    requests \
    jsonschema

# ------------------------------------------------------------------
# 3. Zephyr SDK (xtensa-esp32s3 toolchain + cmake scaffolding)
# ------------------------------------------------------------------
# The SDK 1.0.1 _gnu bundle (~2 GB) is unreliable to download in CI.
# We use a two-step approach:
#   1. The full _gnu bundle for the small cmake scaffolding (< 20 MB)
#      (cmake/ comes first in the tar, so extraction is fast)
#   2. The individual ESP32 and ESP32-S3 toolchain tarballs extracted
#      on top, ensuring the toolchains are always complete.
ENV ZEPHYR_SDK_VERSION=1.0.1
ENV ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk

RUN ARCH="$(uname -m)" \
    && case "${ARCH}" in \
        x86_64) SDK_ARCH="x86_64" ;; \
        aarch64|arm64) SDK_ARCH="aarch64" ;; \
        *) echo "Unsupported architecture: ${ARCH}" && exit 1 ;; \
    esac \
    && TOOLCHAIN_ESP32S3="xtensa-espressif_esp32s3_zephyr-elf" \
    && TOOLCHAIN_ESP32="xtensa-espressif_esp32_zephyr-elf" \
    && FULL_SDK_URL="https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION}/zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-${SDK_ARCH}_gnu.tar.xz" \
    && TOOLCHAIN_ESP32S3_URL="https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION}/toolchain_gnu_linux-${SDK_ARCH}_${TOOLCHAIN_ESP32S3}.tar.xz" \
    && TOOLCHAIN_ESP32_URL="https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION}/toolchain_gnu_linux-${SDK_ARCH}_${TOOLCHAIN_ESP32}.tar.xz" \
    # 3a. Full SDK bundle (cmake scaffolding, small files first in tar)
    && echo "Downloading SDK cmake scaffolding..." \
    && wget -q --show-progress --tries=5 --timeout=120 -O /tmp/sdk-full.tar.xz "${FULL_SDK_URL}" \
    && tar -xf /tmp/sdk-full.tar.xz -C /opt/ \
    && [ -d "/opt/zephyr-sdk-${ZEPHYR_SDK_VERSION}" ] \
    && mv "/opt/zephyr-sdk-${ZEPHYR_SDK_VERSION}" "${ZEPHYR_SDK_INSTALL_DIR}" \
    && rm /tmp/sdk-full.tar.xz \
    # 3b. Overlay the ESP32-S3 toolchain from its individual tarball (guarantees it's complete)
    && echo "Downloading ESP32-S3 toolchain..." \
    && wget -q --show-progress --tries=5 --timeout=120 -O /tmp/toolchain-s3.tar.xz "${TOOLCHAIN_ESP32S3_URL}" \
    && mkdir -p "${ZEPHYR_SDK_INSTALL_DIR}/gnu" \
    && rm -rf "${ZEPHYR_SDK_INSTALL_DIR}/gnu/${TOOLCHAIN_ESP32S3}" \
    && tar -xf /tmp/toolchain-s3.tar.xz -C "${ZEPHYR_SDK_INSTALL_DIR}/gnu/" \
    && rm /tmp/toolchain-s3.tar.xz \
    # 3c. Overlay the ESP32 toolchain from its individual tarball
    && echo "Downloading ESP32 toolchain..." \
    && wget -q --show-progress --tries=5 --timeout=120 -O /tmp/toolchain-esp32.tar.xz "${TOOLCHAIN_ESP32_URL}" \
    && rm -rf "${ZEPHYR_SDK_INSTALL_DIR}/gnu/${TOOLCHAIN_ESP32}" \
    && tar -xf /tmp/toolchain-esp32.tar.xz -C "${ZEPHYR_SDK_INSTALL_DIR}/gnu/" \
    && rm /tmp/toolchain-esp32.tar.xz \
    # 3d. Strip all other toolchains (save ~2 GB)
    && if [ -d "${ZEPHYR_SDK_INSTALL_DIR}/gnu" ]; then \
         for dir in "${ZEPHYR_SDK_INSTALL_DIR}"/gnu/*/; do \
           [ -d "$dir" ] || continue; \
           tc="$(basename "$dir")"; \
           [ "$tc" = "${TOOLCHAIN_ESP32S3}" ] || [ "$tc" = "${TOOLCHAIN_ESP32}" ] && continue; \
           echo "Removing unused toolchain: $tc"; \
           rm -rf "$dir"; \
         done; \
       fi \
    # 3e. Update sdk_gnu_toolchains to list only the ones we keep
    && printf "%s\n%s\n" "${TOOLCHAIN_ESP32S3}" "${TOOLCHAIN_ESP32}" > "${ZEPHYR_SDK_INSTALL_DIR}/sdk_gnu_toolchains" \
    # 3f. Verify the toolchains are usable before proceeding
    && TOOLCHAIN_ESP32S3_BIN="${ZEPHYR_SDK_INSTALL_DIR}/gnu/${TOOLCHAIN_ESP32S3}/bin/${TOOLCHAIN_ESP32S3}-gcc" \
    && TOOLCHAIN_ESP32_BIN="${ZEPHYR_SDK_INSTALL_DIR}/gnu/${TOOLCHAIN_ESP32}/bin/${TOOLCHAIN_ESP32}-gcc" \
    && if [ ! -x "${TOOLCHAIN_ESP32S3_BIN}" ] || [ ! -x "${TOOLCHAIN_ESP32_BIN}" ]; then \
         echo "ERROR: Toolchain not found or not executable"; \
         ls -la "${ZEPHYR_SDK_INSTALL_DIR}/gnu/" 2>/dev/null || echo "gnu/ empty or missing"; \
         exit 1; \
       fi \
    && echo "SDK installed successfully (${TOOLCHAIN_ESP32S3} and ${TOOLCHAIN_ESP32} verified)"

# ------------------------------------------------------------------
# 4. Environment setup
# ------------------------------------------------------------------
ENV ZEPHYR_TOOLCHAIN_VARIANT=zephyr
ENV ZEPHYR_BASE=/workspaces/horcrux/zephyr

# ------------------------------------------------------------------
# 5. Non-root user
# ------------------------------------------------------------------
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000

# Create group first, then user, then assign sudo
RUN groupadd --gid "${USER_GID}" "${USERNAME}" 2>/dev/null || true; \
    if ! id -u "${USERNAME}" &>/dev/null; then \
        if getent passwd "${USER_UID}" &>/dev/null; then \
            useradd --uid "${USER_UID}" --gid "${USER_GID}" -m "${USERNAME}" -o 2>/dev/null || true; \
        else \
            useradd --uid "${USER_UID}" --gid "${USER_GID}" -m "${USERNAME}"; \
        fi; \
    fi; \
    echo "${USERNAME} ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/"${USERNAME}" && \
    chmod 0440 /etc/sudoers.d/"${USERNAME}"

# Entrypoint — adjusts UID/GID at runtime for volume-mounted workspaces.
# Must run as root so it can call usermod/groupmod/chown, then
# it drops privileges to USERNAME via sudo -E before exec.
# Path is relative to build context (workspace root), set in devcontainer.json
COPY .devcontainer/entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

# Switch back to root for entrypoint; the script will drop privileges
USER root
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
WORKDIR /workspaces/horcrux/horcrux-connect
