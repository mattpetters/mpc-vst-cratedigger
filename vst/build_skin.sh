#!/usr/bin/env bash
# Build the MPC skin into vst/build/skin/ (browser-rendered artwork, in Docker).
set -euo pipefail
cd "$(dirname "$0")/.."
MPC_VST="${MPC_VST:-$HOME/mpc-vst}"
docker build -q -t mpc-vst-html-art "$MPC_VST/tools/html_art" >/dev/null
rm -rf vst/build/skin
docker run --rm -u "$(id -u):$(id -g)" -e HOME=/tmp -e MPC_VST=/mpcvst ${SHADOW_SKIN_MPC_OS:+-e SHADOW_SKIN_MPC_OS="$SHADOW_SKIN_MPC_OS"} -v "$PWD":/repo -v "$MPC_VST":/mpcvst:ro \
  -w /repo mpc-vst-html-art:latest python3 vst/gen_skin.py
