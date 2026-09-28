#!/usr/bin/env python3
"""Draws images/sync_off.png (light grey) and images/sync_on.png (amber) for the TRANSPORT SYNC button.

The label is baked in dark ink (the skin builder's own button label is light, unreadable on light grey).
Needs Pillow and Titillium Web SemiBold (OFL): FONT=<path to TitilliumWeb-SemiBold.ttf>, e.g. from
mpc-vst's tools/html_art/fonts/.  Run:  FONT=... python3 vst/make_sync_images.py
"""
import os
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
FONT = os.environ["FONT"]
S, W, H = 4, 176, 48


def make(name, base):
    im = Image.new("RGBA", (W * S, H * S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    shade = tuple(int(c * 0.55) for c in base)
    d.rounded_rectangle((0, 3 * S, W * S - 1, H * S - 1), 6 * S, fill=shade + (255,))
    d.rounded_rectangle((0, 0, W * S - 1, (H - 3) * S), 6 * S, fill=base + (255,))
    hi = tuple(min(255, int(c * 1.12)) for c in base)
    d.rounded_rectangle((2 * S, 2 * S, W * S - 3 * S, (H - 5) * S), 5 * S, outline=hi + (255,), width=S)
    f = ImageFont.truetype(FONT, 17 * S)
    txt = "TRANSPORT SYNC"
    tw = d.textlength(txt, font=f)
    d.text(((W * S - tw) / 2, (H - 3) * S / 2), txt, font=f, fill=(58, 53, 43, 255), anchor="lm")
    im.resize((W, H), Image.LANCZOS).save(os.path.join(HERE, "images", name))


make("sync_off.png", (196, 193, 184))
make("sync_on.png", (232, 163, 23))
