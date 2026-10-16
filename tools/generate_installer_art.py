#!/usr/bin/env python3
"""Generate the installer artwork from the application icon.

Writes transparent PNGs into installer/art so they sit on both the light and
the dark installer background:
  wizard-<w>.png  side image of the welcome and finished pages (164:314),
                  one per Windows scaling level
  small-<n>.png   icon for the header of the inner pages

Usage: python tools/generate_installer_art.py
"""
from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw

from generate_brand_icon import render

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "installer" / "art"
# Image areas for 100%-250% scaling (Inno Setup WizardImageFile).
WIZARD_SIZES = [(202, 386), (269, 515), (336, 643), (403, 772), (498, 953)]
SMALL_SIZES = [55, 69, 83, 110, 138]
ACCENT = (25, 118, 243)


def wizard(width: int, height: int) -> Image.Image:
    scale = 4
    big = Image.new("RGBA", (width * scale, height * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(big)
    cx, cy = big.width / 2, big.height * 0.40
    # Two faint rings behind the icon, in the brand blue.
    for radius, alpha in ((0.46, 22), (0.34, 34)):
        r = big.width * radius
        draw.ellipse((cx - r, cy - r, cx + r, cy + r), fill=ACCENT + (alpha,))
    icon_size = round(width * 0.50) * scale
    icon = render(max(48, icon_size // scale)).resize((icon_size, icon_size), Image.Resampling.LANCZOS)
    big.alpha_composite(icon, (round(cx - icon_size / 2), round(cy - icon_size / 2)))
    return big.resize((width, height), Image.Resampling.LANCZOS)


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    for width, height in WIZARD_SIZES:
        wizard(width, height).save(OUT / f"wizard-{width}.png")
    for size in SMALL_SIZES:
        render(size if size >= 48 else 48).resize((size, size), Image.Resampling.LANCZOS).save(OUT / f"small-{size}.png")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
