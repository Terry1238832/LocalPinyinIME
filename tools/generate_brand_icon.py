#!/usr/bin/env python3
"""Generate the LocalPinyinIME application icon from its vector definition.

The mark is the approved Concept A design: a blue rounded tile, a white speech
bubble holding a blue "P" and a text caret, and a keyboard below the bubble.
The geometry below is a vector redrawing of that design, so every size can be
rendered sharply instead of downscaling one raster master.

Large sizes (48 px and up) draw the full design. Small sizes drop the keyboard
and enlarge the bubble and glyph, keeping strokes at least about 1.5 px wide.

Outputs (in --output-dir):
  LocalPinyinIME.ico            16-256 px, uncompressed 32-bit BGRA DIB entries
  LocalPinyinIME-master.png     1024 px preview/master
  LocalPinyinIME.svg            vector source used by the settings interface
  small/LocalPinyinIME-<n>.png  16, 20, 24, 32 and 40 px renders
Requires Pillow at authoring time only.
"""
from __future__ import annotations

import argparse
import math
import struct
from dataclasses import dataclass
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

ICO_SIZES = [16, 20, 24, 32, 40, 48, 64, 128, 256]
SMALL_SIZES = [16, 20, 24, 32, 40]
SUPERSAMPLE = 8
TILE_LIGHT = (26, 160, 255)    # top right
TILE_DEEP = (8, 76, 214)       # bottom left
TILE_RIM = (110, 225, 255)     # thin highlight along the top right edge
GLYPH = (19, 110, 245)
BUBBLE_TOP = (255, 255, 255)
BUBBLE_BOTTOM = (226, 233, 246)
KEY = (128, 176, 255)
WHITE = (255, 255, 255)

# Full design, in units of the tile width (the tile is the unit square).
BUBBLE = (0.163, 0.154, 0.846, 0.618)
BUBBLE_RADIUS = 0.118
TAIL = ((0.495, 0.600), (0.668, 0.600), (0.584, 0.705))
STROKE = 0.050
# The P is one open stroke: top bar, bowl, return, rounded corner, stem.
P_TOP_Y, P_BAR_LEFT, P_BOWL_X = 0.277, 0.389, 0.500
P_BOWL_R = 0.0665
P_CORNER_X, P_CORNER_R = 0.423, 0.046
P_STEM_X, P_STEM_BOTTOM = 0.377, 0.512
CARET_BOX = (0.664, 0.253, 0.689, 0.538)
KEY_ROWS = [
    (0.692, 0.753, [(0.160 + i * 0.1029, 0.0673) for i in range(7)]),
    (0.789, 0.849, [(0.208, 0.0673), (0.311, 0.0673), (0.415, 0.174), (0.624, 0.0673), (0.727, 0.0673)]),
]
DIM_KEY = (0, 4)  # the key just below the bubble tail
KEY_RADIUS = 0.016


@dataclass
class Layout:
    tile: tuple[float, float, float, float]   # pixels
    bubble: tuple[float, float, float, float]  # tile units
    bubble_radius: float
    tail: tuple[tuple[float, float], ...]
    glyph_scale: float
    glyph_offset: tuple[float, float]
    min_stroke_px: float
    keyboard: bool
    shadow: bool


def layout(size: int) -> Layout:
    inset = size * 0.0625 if size >= 48 else (1 if size >= 32 else 0)
    tile = (inset, inset, size - inset, size - inset)
    if size >= 48:
        return Layout(tile, BUBBLE, BUBBLE_RADIUS, TAIL, 1.0, (0.0, 0.0), 0.0, True, True)
    # Small sizes: a larger bubble centred in the tile and a larger glyph.
    bubble = (0.10, 0.13, 0.90, 0.76)
    scale = {16: 1.75, 20: 1.7, 24: 1.62, 32: 1.5}.get(size, 1.45)
    # Centre the glyph group (stem left edge to caret right edge) in the bubble.
    group = (P_STEM_X - STROKE / 2, P_TOP_Y - STROKE / 2, CARET_BOX[2], P_STEM_BOTTOM + STROKE / 2)
    cx, cy = (bubble[0] + bubble[2]) / 2, (bubble[1] + bubble[3]) / 2
    gx, gy = (group[0] + group[2]) / 2, (group[1] + group[3]) / 2
    offset = (cx - gx * scale, cy - gy * scale)
    tail = ((0.46, 0.74), (0.66, 0.74), (0.56, 0.90))
    # Whole-pixel stroke widths keep the P crisp at taskbar sizes.
    stroke_px = 2 if size <= 24 else 3
    return Layout(tile, bubble, 0.17, tail, scale, offset, stroke_px, False, size >= 32)


def gradient(width: int, height: int) -> Image.Image:
    """Diagonal gradient: light at the top right, deep at the bottom left."""
    image = Image.new("RGB", (width, height))
    pixels = image.load()
    for y in range(height):
        for x in range(width):
            t = ((width - 1 - x) * 0.45 + y * 0.55) / max(1, (width - 1) * 0.45 + (height - 1) * 0.55)
            pixels[x, y] = tuple(round(a + (b - a) * t) for a, b in zip(TILE_LIGHT, TILE_DEEP))
    return image


def vertical(width: int, height: int, top: tuple, bottom: tuple) -> Image.Image:
    image = Image.new("RGB", (width, height))
    draw = ImageDraw.Draw(image)
    for y in range(height):
        t = y / max(1, height - 1)
        draw.line((0, y, width, y), fill=tuple(round(a + (b - a) * t) for a, b in zip(top, bottom)))
    return image


def p_path(steps: int = 48) -> list[tuple[float, float]]:
    """Centre line of the P stroke, in tile units of the full design."""
    points = [(P_BAR_LEFT, P_TOP_Y), (P_BOWL_X, P_TOP_Y)]
    cy = P_TOP_Y + P_BOWL_R
    for i in range(1, steps + 1):
        a = -math.pi / 2 + math.pi * i / steps
        points.append((P_BOWL_X + P_BOWL_R * math.cos(a), cy + P_BOWL_R * math.sin(a)))
    bottom = P_TOP_Y + 2 * P_BOWL_R
    points.append((P_CORNER_X, bottom))
    ccx, ccy = P_CORNER_X, bottom + P_CORNER_R
    for i in range(1, steps // 2 + 1):
        a = -math.pi / 2 - (math.pi / 2) * i / (steps // 2)
        points.append((ccx + P_CORNER_R * math.cos(a), ccy + P_CORNER_R * math.sin(a)))
    points.append((P_STEM_X, P_STEM_BOTTOM))
    return points


def render(size: int) -> Image.Image:
    lay = layout(size)
    s = SUPERSAMPLE
    big = size * s
    left, top, right, bottom = (value * s for value in lay.tile)
    width = right - left

    def tile_point(x: float, y: float) -> tuple[float, float]:
        return (left + x * width, top + y * width)

    snap = size < 48
    def glyph_point(x: float, y: float) -> tuple[float, float]:
        px, py = tile_point(lay.glyph_offset[0] + x * lay.glyph_scale, lay.glyph_offset[1] + y * lay.glyph_scale)
        if not snap:
            return px, py
        # Stroke centres on whole pixels (even widths) or pixel centres (odd).
        half = 0.0 if round(lay.min_stroke_px) % 2 == 0 else 0.5
        return ((round(px / s - half) + half) * s, (round(py / s - half) + half) * s)

    image = Image.new("RGBA", (big, big), (0, 0, 0, 0))
    tile_mask = Image.new("L", (big, big), 0)
    ImageDraw.Draw(tile_mask).rounded_rectangle((left, top, right, bottom), radius=width * 0.22, fill=255)
    image.paste(gradient(big, big), (0, 0), tile_mask)
    # Keyboard (large sizes only).
    if lay.keyboard:
        keys = Image.new("L", (big, big), 0)
        dim = Image.new("L", (big, big), 0)
        for row, (y0, y1, items) in enumerate(KEY_ROWS):
            for index, (x0, w) in enumerate(items):
                box = (*tile_point(x0, y0), *tile_point(x0 + w, y1))
                target = dim if (row, index) == DIM_KEY else keys
                ImageDraw.Draw(target).rounded_rectangle(box, radius=KEY_RADIUS * width, fill=255)
        image.paste(Image.new("RGB", (big, big), KEY), (0, 0), keys.point(lambda v: v * 0.78))
        image.paste(Image.new("RGB", (big, big), KEY), (0, 0), dim.point(lambda v: v * 0.30))

    # Speech bubble with its tail, and a soft shadow below it.
    bubble = Image.new("L", (big, big), 0)
    draw = ImageDraw.Draw(bubble)
    bx0, by0 = tile_point(lay.bubble[0], lay.bubble[1])
    bx1, by1 = tile_point(lay.bubble[2], lay.bubble[3])
    draw.rounded_rectangle((bx0, by0, bx1, by1), radius=lay.bubble_radius * width, fill=255)
    tail = [tile_point(*point) for point in lay.tail]
    draw.polygon(tail, fill=255)
    tip = tail[2]
    draw.ellipse((tip[0] - width * 0.012, tip[1] - width * 0.03, tip[0] + width * 0.012, tip[1]), fill=255)
    if lay.shadow:
        shadow = bubble.filter(ImageFilter.GaussianBlur(width * 0.025))
        shadow = Image.composite(shadow, Image.new("L", (big, big), 0), tile_mask)
        offset = Image.new("L", (big, big), 0)
        offset.paste(shadow, (0, round(width * 0.018)))
        image.paste(Image.new("RGB", (big, big), (0, 40, 140)), (0, 0), offset.point(lambda v: v * 0.38))
    image.paste(vertical(big, big, BUBBLE_TOP, BUBBLE_BOTTOM).crop((0, 0, big, big)), (0, 0), bubble)

    # The P stroke and the caret.
    stroke = lay.min_stroke_px * s if snap else STROKE * lay.glyph_scale * width
    glyph = Image.new("L", (big, big), 0)
    draw = ImageDraw.Draw(glyph)
    # Straight runs, true arcs, and round discs at every joint and end.
    r = stroke / 2
    bottom_y = P_TOP_Y + 2 * P_BOWL_R
    joints = [glyph_point(P_BAR_LEFT, P_TOP_Y), glyph_point(P_BOWL_X, P_TOP_Y), glyph_point(P_BOWL_X, bottom_y),
              glyph_point(P_CORNER_X, bottom_y), glyph_point(P_STEM_X, bottom_y + P_CORNER_R),
              glyph_point(P_STEM_X, P_STEM_BOTTOM)]
    for start, end in ((0, 1), (2, 3), (4, 5)):
        draw.line((joints[start], joints[end]), fill=255, width=round(stroke))
    def arc(cx: float, cy: float, radius: float, begin: float, finish: float) -> None:
        x, y = glyph_point(cx, cy)
        rr = abs(glyph_point(cx, cy + radius)[1] - y) if snap else radius * lay.glyph_scale * width
        draw.arc((x - rr - r, y - rr - r, x + rr + r, y + rr + r), begin, finish, fill=255, width=round(stroke))
    arc(P_BOWL_X, P_TOP_Y + P_BOWL_R, P_BOWL_R, -90, 90)
    arc(P_CORNER_X, bottom_y + P_CORNER_R, P_CORNER_R, 180, 270)
    for x, y in joints:
        draw.ellipse((x - r, y - r, x + r, y + r), fill=255)
    cx0, cy0 = glyph_point(CARET_BOX[0], CARET_BOX[1])
    cx1, cy1 = glyph_point(CARET_BOX[2], CARET_BOX[3])
    caret_width = max(1, round(lay.min_stroke_px * 0.6)) * s if snap else cx1 - cx0
    cx1 = cx0 + caret_width
    draw.rounded_rectangle((cx0, cy0, cx1, cy1), radius=caret_width / 2, fill=255)
    image.paste(Image.new("RGB", (big, big), GLYPH), (0, 0), glyph)
    return image.resize((size, size), Image.Resampling.BOX)


def svg() -> str:
    lay = layout(1024)
    left, top, right, _ = lay.tile
    width = right - left
    u = lambda value: value * width
    tp = lambda x, y: f"{left + u(x):.1f} {top + u(y):.1f}"
    bx0, by0, bx1, by1 = BUBBLE
    bottom = P_TOP_Y + 2 * P_BOWL_R
    p = (f"M{tp(P_BAR_LEFT, P_TOP_Y)} L{tp(P_BOWL_X, P_TOP_Y)} "
         f"A{u(P_BOWL_R):.1f} {u(P_BOWL_R):.1f} 0 0 1 {tp(P_BOWL_X, bottom)} L{tp(P_CORNER_X, bottom)} "
         f"A{u(P_CORNER_R):.1f} {u(P_CORNER_R):.1f} 0 0 0 {tp(P_STEM_X, bottom + P_CORNER_R)} L{tp(P_STEM_X, P_STEM_BOTTOM)}")
    keys = []
    for row, (y0, y1, items) in enumerate(KEY_ROWS):
        for index, (x0, w) in enumerate(items):
            opacity = 0.30 if (row, index) == DIM_KEY else 0.78
            keys.append(f'  <rect x="{left + u(x0):.1f}" y="{top + u(y0):.1f}" width="{u(w):.1f}" height="{u(y1 - y0):.1f}" '
                        f'rx="{u(KEY_RADIUS):.1f}" fill="#80B0FF" fill-opacity="{opacity}"/>')
    tail = " ".join(tp(*point) for point in TAIL)
    hexc = lambda rgb: "#%02X%02X%02X" % rgb
    return "\n".join([
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1024 1024" role="img">',
        "  <title>LocalPinyinIME</title>",
        "  <defs>",
        f'    <linearGradient id="tile" x1="1" y1="0" x2="0" y2="1"><stop offset="0" stop-color="{hexc(TILE_LIGHT)}"/><stop offset="1" stop-color="{hexc(TILE_DEEP)}"/></linearGradient>',
        f'    <linearGradient id="bubble" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="{hexc(BUBBLE_TOP)}"/><stop offset="1" stop-color="{hexc(BUBBLE_BOTTOM)}"/></linearGradient>',
        '    <filter id="shadow" x="-20%" y="-20%" width="140%" height="160%"><feDropShadow dx="0" dy="16" stdDeviation="22" flood-color="#00288C" flood-opacity="0.38"/></filter>',
        "  </defs>",
        f'  <rect x="{left:.1f}" y="{top:.1f}" width="{width:.1f}" height="{width:.1f}" rx="{width * 0.22:.1f}" fill="url(#tile)"/>',
        *keys,
        f'  <g filter="url(#shadow)" fill="url(#bubble)"><rect x="{left + u(bx0):.1f}" y="{top + u(by0):.1f}" width="{u(bx1 - bx0):.1f}" height="{u(by1 - by0):.1f}" rx="{u(BUBBLE_RADIUS):.1f}"/>'
        f'<path d="M{tail} Z" stroke="url(#bubble)" stroke-width="{u(0.02):.1f}" stroke-linejoin="round"/></g>',
        f'  <path d="{p}" fill="none" stroke="{hexc(GLYPH)}" stroke-width="{u(STROKE):.1f}" stroke-linecap="round" stroke-linejoin="round"/>',
        f'  <rect x="{left + u(CARET_BOX[0]):.1f}" y="{top + u(CARET_BOX[1]):.1f}" width="{u(CARET_BOX[2] - CARET_BOX[0]):.1f}" height="{u(CARET_BOX[3] - CARET_BOX[1]):.1f}" rx="{u(CARET_BOX[2] - CARET_BOX[0]) / 2:.1f}" fill="{hexc(GLYPH)}"/>',
        "</svg>",
        "",
    ])


def encode_icon_dib(image: Image.Image) -> bytes:
    """A 32-bit BGRA DIB icon entry with an explicit alpha-derived AND mask."""
    image = image.convert("RGBA")
    width, height = image.size
    header = struct.pack("<IiiHHIIiiII", 40, width, height * 2, 1, 32, 0, width * height * 4, 0, 0, 0, 0)
    pixels = bytearray()
    source = image.load()
    for y in range(height - 1, -1, -1):
        for x in range(width):
            r, g, b, a = source[x, y]
            pixels.extend((b, g, r, a))
    mask_stride = ((width + 31) // 32) * 4
    mask = bytearray(mask_stride * height)
    for y in range(height - 1, -1, -1):
        row = height - 1 - y
        for x in range(width):
            if source[x, y][3] == 0:
                mask[row * mask_stride + x // 8] |= 0x80 >> (x % 8)
    return header + bytes(pixels) + bytes(mask)


def write_ico(frames: list[Image.Image], path: Path) -> None:
    entries = [(frame.width, encode_icon_dib(frame)) for frame in frames]
    offset = 6 + 16 * len(entries)
    directory = bytearray()
    payload = bytearray()
    for size, data in entries:
        directory.extend(struct.pack("<BBBBHHII", 0 if size == 256 else size, 0 if size == 256 else size,
                                     0, 0, 1, 32, len(data), offset))
        payload.extend(data)
        offset += len(data)
    path.write_bytes(struct.pack("<HHH", 0, 1, len(entries)) + directory + payload)


def contact_sheet(frames: list[Image.Image], background: tuple[int, int, int, int]) -> Image.Image:
    width = sum(frame.width for frame in frames) + 16 * (len(frames) + 1)
    sheet = Image.new("RGBA", (width, 256 + 32), background)
    x = 16
    for frame in frames:
        sheet.alpha_composite(frame, (x, 16 + (256 - frame.height) // 2))
        x += frame.width + 16
    return sheet


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--contact-sheet", type=Path, help="optional preview on light and dark backgrounds")
    args = parser.parse_args()
    out = args.output_dir
    (out / "small").mkdir(parents=True, exist_ok=True)
    frames = [render(size) for size in ICO_SIZES]
    write_ico(frames, out / "LocalPinyinIME.ico")
    render(1024).save(out / "LocalPinyinIME-master.png")
    (out / "LocalPinyinIME.svg").write_text(svg(), encoding="utf-8", newline="\n")
    for size in SMALL_SIZES:
        render(size).save(out / "small" / f"LocalPinyinIME-{size}.png")
    if args.contact_sheet:
        light = contact_sheet(frames, (243, 243, 243, 255))
        dark = contact_sheet(frames, (32, 32, 32, 255))
        sheet = Image.new("RGBA", (light.width, light.height * 2))
        sheet.paste(light, (0, 0))
        sheet.paste(dark, (0, light.height))
        sheet.save(args.contact_sheet)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
