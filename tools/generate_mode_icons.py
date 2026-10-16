#!/usr/bin/env python3
"""Generate the fallback 中/英 input-mode icons (assets/mode-icons/*.ico).

The input method normally draws these glyphs at runtime in the taskbar's own
color (src/ime/language_bar.cpp). These resource icons are used only if that
fails, so they use a brand-blue tile with a white glyph that reads on light
and dark taskbars alike. Requires Pillow and the Windows Microsoft YaHei UI font.
"""
from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

from generate_brand_icon import write_ico

SIZES = [16, 20, 24, 32, 40, 48]
FONT = Path(r"C:\Windows\Fonts\msyh.ttc")
GLYPHS = {"mode_zh.ico": "中", "mode_en.ico": "英"}


def render(glyph: str, size: int) -> Image.Image:
    scale = 8
    big = size * scale
    tile = Image.new("L", (big, big), 0)
    ImageDraw.Draw(tile).rounded_rectangle((0, 0, big - 1, big - 1), radius=big * 0.22, fill=255)
    font = ImageFont.truetype(str(FONT), int(big * 0.74), index=1)
    mask = Image.new("L", (big, big), 0)
    draw = ImageDraw.Draw(mask)
    left, top, right, bottom = draw.textbbox((0, 0), glyph, font=font)
    draw.text(((big - (right - left)) / 2 - left, (big - (bottom - top)) / 2 - top), glyph, font=font, fill=255)
    image = Image.new("RGBA", (big, big), (0, 0, 0, 0))
    image.paste(Image.new("RGBA", (big, big), (25, 118, 243, 255)), (0, 0), tile)
    image.paste(Image.new("RGBA", (big, big), (255, 255, 255, 255)), (0, 0), mask)
    return image.resize((size, size), Image.Resampling.BOX)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for name, glyph in GLYPHS.items():
        write_ico([render(glyph, size) for size in SIZES], args.output_dir / name)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
