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
    psutil

# ------------------------------------------------------------------
# 3. Zephyr SDK (includes xtensa-esp32s3 toolchain)
# ------------------------------------------------------------------
ENV ZEPHYR_SDK_VERSION=0.16.8
ENV ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk

RUN ARCH="$(uname -m)" \
    && case "${ARCH}" in \
        x86_64) SDK_ARCH="x86_64" ;; \
        aarch64|arm64) SDK_ARCH="aarch64" ;; \
        *) echo "Unsupported architecture: ${ARCH}" && exit 1 ;; \
    esac \
    && wget -q "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION}/zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-${SDK_ARCH}.tar.xz" \
    -O /tmp/zephyr-sdk.tar.xz \
    && tar -xf /tmp/zephyr-sdk.tar.xz -C /opt/ \
    && mv "/opt/zephyr-sdk-${ZEPHYR_SDK_VERSION}" "${ZEPHYR_SDK_INSTALL_DIR}" \
    && rm /tmp/zephyr-sdk.tar.xz \
    && cd "${ZEPHYR_SDK_INSTALL_DIR}" \
    && ./setup.sh -t xtensa-espressif_esp32s3_zephyr-elf \
    && rm -rf "${ZEPHYR_SDK_INSTALL_DIR}"/sysroots/*/usr/share/doc \
    && rm -rf "${ZEPHYR_SDK_INSTALL_DIR}"/*.tar.* \
    && for dir in "${ZEPHYR_SDK_INSTALL_DIR}"/*/; do \
        tc="$(basename "$dir")"; \
        [ "$tc" = "xtensa-espressif_esp32s3_zephyr-elf" ] && continue; \
        [ "$tc" = "cmake" ] && continue; \
        [ "$tc" = "hosttools" ] && continue; \
        [ "$tc" = "sysroots" ] && continue; \
        echo "Removing unused toolchain: $tc"; \
        rm -rf "$dir"; \
    done

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
WORKDIR /workspaces/horcrux/apsta_mode
