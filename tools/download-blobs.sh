#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
#
# Télécharge les blobs binaires Espressif nécessaires au driver Wi-Fi ESP32.
set -euo pipefail

HAL_DIR="/workspaces/horcrux/modules/hal/espressif"
MODULE_YML="$HAL_DIR/zephyr/module.yml"

if [ ! -f "$MODULE_YML" ]; then
  echo "⚠️  Module HAL Espressif introuvable — west update a-t-il été lancé ?"
  exit 0
fi

echo "📦 Téléchargement des blobs Espressif…"

# Extrait les URLs des blobs du module.yml et les télécharge
# Les paths dans module.yml sont relatifs (ex: lib/esp32c6/libble_app.a).
# Zephyr CMake les attend sous zephyr/blobs/ (convention du module HAL).
python3 -c "
import yaml, os, subprocess

with open('$MODULE_YML') as f:
    mod = yaml.safe_load(f)

for blob in mod.get('blobs', []):
    path = os.path.join('$HAL_DIR', 'zephyr', 'blobs', blob['path'])
    if os.path.exists(path) and os.path.getsize(path) > 0:
        continue  # déjà présent
    os.makedirs(os.path.dirname(path), exist_ok=True)
    print(f'  ↓ {blob[\"path\"]}')
    subprocess.run(['wget', '-q', blob['url'], '-O', path], check=True)
" 2>&1 || echo "⚠️  Certains blobs n'ont pas pu être téléchargés (non bloquant)."

echo "✅ Téléchargement des blobs terminé"
