#!/usr/bin/env bash
# Everything a release zip needs, in one go (mpc-vst-plugins' vst-release workflow runs this):
#   vst/build/cratedigger.so                       -> payload/vst/
#   vst/build/skin/sd88me - VST - Crate Digger/    -> payload/Synths/
#   vst/build/pluginlist-entry.xml
#   vst/build/engine/bin/                          -> payload/vst/cratedigger/bin (release.py --extra):
#       yt-dlp, ffmpeg/ffprobe, the private Python 3.11 and zlib module, yt_dlp_daemon.py
# Needs Docker with armhf emulation, zig, python3 with Pillow, and an mpc-vst-plugins checkout
# (MPC_VST) for its skin tooling. The skin artwork is drawn by the browser renderer
# (tools/html_art.py, "mpc-vst-html-art" Docker image: headless Chromium + Pillow) for real
# Titillium Web text, knob value arcs and transparent-edge controls -- not published to any
# registry, so build it here from $MPC_VST/tools/html_art/Dockerfile (cached by tag after the
# first build, on a dev machine and in CI alike).
set -euo pipefail
cd "$(dirname "$0")/.."
MPC_VST="${MPC_VST:?set MPC_VST to an mpc-vst-plugins checkout}"

scripts/build-deps.sh
scripts/build-pyzlib.sh
scripts/build-python.sh
vst/build.sh

docker build -q -t mpc-vst-html-art "$MPC_VST/tools/html_art"
docker run --rm -e MPC_VST=/mpcvst -v "$PWD":/repo -v "$MPC_VST":/mpcvst \
  -w /repo mpc-vst-html-art:latest python3 vst/gen_skin.py

rm -rf vst/build/engine
mkdir -p vst/build/engine/bin
cp -R build/deps/bin/. vst/build/engine/bin/
cp src/bin/yt_dlp_daemon.py vst/build/engine/bin/
ls -la vst/build vst/build/engine/bin
