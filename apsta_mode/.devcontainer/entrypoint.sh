#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
#
# Entrypoint — Adjusts the container user UID/GID to match the workspace
# volume owner, so that 'west init', 'west build' etc. work regardless
# of the host user's UID.
#
# This pattern is used by many devcontainer base images (e.g.
# mcr.microsoft.com/devcontainers/base).

set -eu

USERNAME="${USERNAME:-vscode}"
USER_UID="${USER_UID:-1000}"
USER_GID="${USER_GID:-1000}"
WORKSPACE="${WORKSPACE:-/workspaces/horcrux}"

# ---- Detect the owner of the workspace mount ---------------------------
if [ -d "$WORKSPACE" ]; then
    MOUNT_UID="$(stat -c '%u' "$WORKSPACE")"
    MOUNT_GID="$(stat -c '%g' "$WORKSPACE")"
else
    MOUNT_UID="$USER_UID"
    MOUNT_GID="$USER_GID"
fi

# ---- (Re)create the user with the detected IDs -------------------------
if [ "$MOUNT_GID" -ne "$(id -g "$USERNAME" 2>/dev/null || echo 0)" ]; then
    groupadd --gid "$MOUNT_GID" "$USERNAME" 2>/dev/null || \
        groupmod -g "$MOUNT_GID" "$USERNAME"
fi

if [ "$MOUNT_UID" -ne "$(id -u "$USERNAME" 2>/dev/null || echo 0)" ]; then
    usermod -u "$MOUNT_UID" -g "$MOUNT_GID" "$USERNAME" 2>/dev/null || true
fi

# Make sure home directory has correct ownership
chown -R "$MOUNT_UID:$MOUNT_GID" "/home/$USERNAME" 2>/dev/null || true

# ---- Fix ownership of Zephyr SDK (installed as root) -------------------
if [ -d "$ZEPHYR_SDK_INSTALL_DIR" ]; then
    chown -R "$MOUNT_UID:$MOUNT_GID" "$ZEPHYR_SDK_INSTALL_DIR" 2>/dev/null || true
fi

# ---- Pre-fetch ESP32 blobs (idempotent) --------------------------------
# Runs before every command so CI docker run --rm also gets blobs without
# needing an explicit CI step.  If the west workspace isn't initialized
# yet the script is a no-op.
BLOB_SCRIPT="/workspaces/horcrux/apsta_mode/tools/download-blobs.sh"
if [ -f "$BLOB_SCRIPT" ]; then
    bash "$BLOB_SCRIPT" 2>/dev/null || true
fi

# ---- Drop privileges and execute the requested command -----------------
# We run as root; drop to the adjusted user before exec.
# -E  preserves env vars (ZEPHYR_BASE, SDK path, toolchain variant…)
# -H  resets HOME to the target user's home (avoids west reading
#     /root/.westconfig as a non-root user).
exec sudo -E -H -u "$USERNAME" "$@"
