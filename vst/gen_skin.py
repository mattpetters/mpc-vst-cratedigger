#!/usr/bin/env python3
"""layout.conf + params.json -> build/skin/<vendor> - VST - <name>/ (mpc-vst-plugins' shadow_skin.py + html_art.py).

Needs Pillow and Playwright's Chromium, so run it through vst/build_skin.sh (Docker), or in the
mpc-vst-html-art image. MPC_VST points at an mpc-vst-plugins checkout (default ~/mpc-vst).
"""
import json
import os
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
MPC_VST = os.environ.get("MPC_VST") or os.path.expanduser("~/mpc-vst")
sys.path.insert(0, os.path.join(MPC_VST, "tools"))
import shadow_skin  # noqa: E402

spec = json.load(open(os.path.join(ROOT, "params.json")))
art = os.environ.get("SHADOW_ART") or os.path.join(MPC_VST, "tools", "html_art.py")
print("skin:", shadow_skin.write_skin(os.path.join(ROOT, "build", "skin"), spec["vendor"], spec["name"],
                                      os.path.join(ROOT, "layout.conf"), spec["params"], art))
