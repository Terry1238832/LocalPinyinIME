#!/usr/bin/env python3
import argparse
import io
import json
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image

EXPECTED_ICO_SIZES = {16, 20, 24, 32, 40, 48, 64, 128, 256}
EXPECTED_COLORS = {"#0D47A1", "#1976F3", "#4DD0E1", "#6B7280", "#F8FAFC"}


def read_ico_sizes(path: Path) -> set[int]:
    data = path.read_bytes()
    reserved, icon_type, count = struct.unpack_from("<HHH", data, 0)
    if reserved != 0 or icon_type != 1:
        raise AssertionError(f"{path} is not a Windows icon file")
    sizes = set()
    offset = 6
    for _ in range(count):
        width, height = struct.unpack_from("<BB", data, offset)
        sizes.add(256 if width == 0 else width)
        sizes.add(256 if height == 0 else height)
        offset += 16
    return sizes


def read_ico_entries(path: Path) -> dict[int, bytes]:
    data = path.read_bytes()
    reserved, icon_type, count = struct.unpack_from("<HHH", data, 0)
    if reserved != 0 or icon_type != 1:
        raise AssertionError(f"{path} is not a Windows icon file")
    entries: dict[int, bytes] = {}
    offset = 6
    for _ in range(count):
        width, height, _, _, _, _, byte_count, image_offset = struct.unpack_from("<BBBBHHII", data, offset)
        size = 256 if width == 0 else width
        if size != (256 if height == 0 else height):
            raise AssertionError(f"non-square ICO entry in {path}")
        entries[size] = data[image_offset:image_offset + byte_count]
        offset += 16
    return entries


def decode_icon_entry(data: bytes) -> Image.Image:
    if data.startswith(b"\x89PNG\r\n\x1a\n"):
        return Image.open(io.BytesIO(data)).convert("RGBA")
    if len(data) < 40:
        raise AssertionError("ICO entry is too small to be a DIB")
    header_size, width, doubled_height, planes, bit_count, compression, *_ = struct.unpack_from("<IiiHHIIiiII", data, 0)
    if header_size < 40 or bit_count != 32 or compression != 0:
        raise AssertionError(f"ICO DIB entry must be uncompressed 32-bit BGRA, got bit_count={bit_count}, compression={compression}")
    height = doubled_height // 2
    pixel_offset = header_size
    row_bytes = width * 4
    pixel_bytes = data[pixel_offset:pixel_offset + row_bytes * height]
    image = Image.new("RGBA", (width, height))
    pixels = image.load()
    for y in range(height):
        source_y = height - 1 - y
        row = pixel_bytes[source_y * row_bytes:(source_y + 1) * row_bytes]
        for x in range(width):
            blue, green, red, alpha = row[x * 4:x * 4 + 4]
            pixels[x, y] = (red, green, blue, alpha)
    return image


def assert_clean_icon_edges(image: Image.Image, label: str) -> None:
    image = image.convert("RGBA")
    width, height = image.size
    corners = [
        image.getpixel((0, 0)),
        image.getpixel((width - 1, 0)),
        image.getpixel((0, height - 1)),
        image.getpixel((width - 1, height - 1)),
    ]
    if any(alpha != 0 for *_, alpha in corners):
        raise AssertionError(f"{label} must have transparent corners, not a white background")

    pixels = image.load()
    seen: set[tuple[int, int]] = set()
    queue: list[tuple[int, int]] = []

    def is_edge_white(x: int, y: int) -> bool:
        red, green, blue, alpha = pixels[x, y]
        return alpha > 12 and min(red, green, blue) > 190 and max(red, green, blue) - min(red, green, blue) < 70

    for x in range(width):
        for y in (0, height - 1):
            if is_edge_white(x, y):
                seen.add((x, y))
                queue.append((x, y))
    for y in range(height):
        for x in (0, width - 1):
            if is_edge_white(x, y) and (x, y) not in seen:
                seen.add((x, y))
                queue.append((x, y))

    while queue:
        x, y = queue.pop()
        for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if 0 <= nx < width and 0 <= ny < height and (nx, ny) not in seen and is_edge_white(nx, ny):
                seen.add((nx, ny))
                queue.append((nx, ny))

    if seen:
        raise AssertionError(f"{label} has edge-connected white/gray matte pixels")


def cmake_target_block(cmake_text: str, target: str) -> str:
    marker = f"add_executable({target}"
    start = cmake_text.find(marker)
    if start < 0:
        raise AssertionError(f"missing target block for {target}")
    next_target = cmake_text.find("\nadd_executable(", start + len(marker))
    next_library = cmake_text.find("\nadd_library(", start + len(marker))
    ends = [pos for pos in [next_target, next_library] if pos >= 0]
    end = min(ends) if ends else len(cmake_text)
    return cmake_text[start:end]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True, type=Path)
    parser.add_argument("--generated-header", required=True, type=Path)
    args = parser.parse_args()
    root = args.root

    icon_dir = root / "assets" / "branding" / "icon"
    ico = icon_dir / "LocalPinyinIME.ico"
    master_png = icon_dir / "LocalPinyinIME-master.png"
    svg = icon_dir / "LocalPinyinIME.svg"
    small_svg = icon_dir / "LocalPinyinIME-small.svg"
    icon_readme = icon_dir / "README.md"
    theme_json = root / "assets" / "branding" / "theme" / "local_pinyin_blue_theme.json"
    rc = root / "src" / "settings" / "LocalPinyinSettings.rc"

    for path in [ico, master_png, svg, small_svg, icon_readme, theme_json, rc, args.generated_header]:
        if not path.exists():
            raise AssertionError(f"missing branding asset: {path}")

    sizes = read_ico_sizes(ico)
    if sizes != EXPECTED_ICO_SIZES:
        raise AssertionError(f"unexpected ICO sizes: {sorted(sizes)}")

    ico_entries = read_ico_entries(ico)
    for size in [16, 20, 24, 32, 40]:
        source_png = icon_dir / "small" / f"LocalPinyinIME-{size}.png"
        uploaded_source_png = icon_dir / "small" / "source" / f"LocalPinyinIME-{size}.png"
        if not source_png.exists():
            raise AssertionError(f"missing official small icon source: {source_png}")
        if not uploaded_source_png.exists():
            raise AssertionError(f"missing uploaded small icon source archive copy: {uploaded_source_png}")
        small_image = Image.open(source_png).convert("RGBA")
        if small_image.size != (size, size):
            raise AssertionError(f"{source_png} must be {size}x{size}")
        assert_clean_icon_edges(small_image, str(source_png))
        ico_image = decode_icon_entry(ico_entries[size])
        if ico_image.size != (size, size):
            raise AssertionError(f"ICO {size}px entry decoded to {ico_image.size}, expected {size}x{size}")
        assert_clean_icon_edges(ico_image, f"ICO {size}px entry")

    for size in [48, 64, 128, 256]:
        ico_image = decode_icon_entry(ico_entries[size])
        if ico_image.size != (size, size):
            raise AssertionError(f"ICO {size}px entry decoded to {ico_image.size}, expected {size}x{size}")
        assert_clean_icon_edges(ico_image, f"ICO {size}px entry")

    readme_text = icon_readme.read_text(encoding="utf-8")
    if "LocalPinyinIME-master.png" not in readme_text or "approved Concept A production master" not in readme_text:
        raise AssertionError("icon README must document the approved PNG production master")
    if "16, 20" not in readme_text or "small/" not in readme_text or "source/" not in readme_text:
        raise AssertionError("icon README must document design-provided small PNG sources")
    for svg_path in [svg, small_svg]:
        svg_text = svg_path.read_text(encoding="utf-8")
        if "data:image/png;base64" not in svg_text or "Production ICO is generated from LocalPinyinIME-master.png" not in svg_text:
            raise AssertionError(f"{svg_path.name} must be a PNG-master wrapper, not a redrawn icon")

    theme = json.loads(theme_json.read_text(encoding="utf-8"))
    color_values = set(theme["colors"].values())
    if color_values != EXPECTED_COLORS:
        raise AssertionError(f"unexpected brand colors: {sorted(color_values)}")

    generated = args.generated_header.read_text(encoding="utf-8")
    for token in [
        "kLocalPinyinBlueDeep_blue",
        "kLocalPinyinBlueAzure_blue",
        "kLocalPinyinBlueCyan_highlight",
        "kLocalPinyinBlueLightWindow_background",
        "kLocalPinyinBlueDarkWindow_background",
    ]:
        if token not in generated:
            raise AssertionError(f"generated theme header is missing {token}")

    with tempfile.TemporaryDirectory() as temp_dir:
        temp_header = Path(temp_dir) / "local_pinyin_blue_theme.h"
        subprocess.run(
            [
                sys.executable,
                str(root / "tools" / "generate_brand_theme_header.py"),
                "--input",
                str(theme_json),
                "--output",
                str(temp_header),
            ],
            check=True,
        )
        if temp_header.read_text(encoding="utf-8") != generated:
            raise AssertionError("generated theme header is not reproducible from the JSON token source")

    rc_text = rc.read_text(encoding="utf-8")
    if "IDI_LOCALPINYINIME_APP ICON" not in rc_text or "LocalPinyinIME.ico" not in rc_text:
        raise AssertionError("settings resource script does not bind the app icon")

    settings_cpp = (root / "src" / "settings" / "settings_window.cpp").read_text(encoding="utf-8")
    for required in [
        "LoadImageW(instance",
        "MAKEINTRESOURCEW(IDI_LOCALPINYINIME_APP)",
        "wc.hIcon = app_icon_big_",
        "wc.hIconSm = app_icon_small_",
        "WM_SETICON, ICON_BIG",
        "WM_SETICON, ICON_SMALL",
    ]:
        if required not in settings_cpp:
            raise AssertionError(f"settings window runtime icon setup is missing {required}")

    cmake_text = (root / "CMakeLists.txt").read_text(encoding="utf-8")
    settings_block = cmake_target_block(cmake_text, "LocalPinyinSettings")
    if "src/settings/LocalPinyinSettings.rc" not in settings_block:
        raise AssertionError("LocalPinyinSettings target does not include the icon resource")

    for helper in ["LocalPinyinImeSetup", "LocalPinyinImeAudit", "LocalPinyinImeEnable"]:
        helper_block = cmake_target_block(cmake_text, helper)
        if "LocalPinyinSettings.rc" in helper_block:
            raise AssertionError(f"{helper} must not receive the user-facing settings icon resource")

    source_text = "\n".join(path.read_text(encoding="utf-8", errors="ignore")
                            for path in (root / "src").rglob("*.*")
                            if path.suffix.lower() in {".cpp", ".h"})
    if "Shell_NotifyIcon" in source_text or "NOTIFYICONDATA" in source_text:
        raise AssertionError("tray icon integration must be audited before adding app icon resources to it")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
