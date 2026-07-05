#!/usr/bin/env python3
import argparse
import base64
import io
import struct
from collections import deque
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont


ICO_SIZES = [16, 20, 24, 32, 40, 48, 64, 128, 256]
SMALL_ASSET_SIZES = {16, 20, 24, 32, 40}
LARGE_MASTER_SIZES = [48, 64, 128, 256]


def trim_transparent_bounds(image: Image.Image) -> tuple[int, int, int, int]:
    alpha = image.getchannel("A")
    bounds = alpha.getbbox()
    if not bounds:
        return (0, 0, image.width, image.height)
    return bounds


def remove_preview_checkerboard(image: Image.Image) -> Image.Image:
    """Remove the neutral checkerboard preview background around the approved PNG."""
    image = image.convert("RGBA")
    width, height = image.size
    pixels = image.load()

    def is_edge_background(x: int, y: int) -> bool:
        r, g, b, a = pixels[x, y]
        return a > 0 and min(r, g, b) >= 232 and max(r, g, b) - min(r, g, b) <= 18

    visited: set[tuple[int, int]] = set()
    queue: deque[tuple[int, int]] = deque()
    for x in range(width):
        for y in (0, height - 1):
            if is_edge_background(x, y):
                visited.add((x, y))
                queue.append((x, y))
    for y in range(height):
        for x in (0, width - 1):
            if is_edge_background(x, y) and (x, y) not in visited:
                visited.add((x, y))
                queue.append((x, y))

    while queue:
        x, y = queue.popleft()
        for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if 0 <= nx < width and 0 <= ny < height and (nx, ny) not in visited and is_edge_background(nx, ny):
                visited.add((nx, ny))
                queue.append((nx, ny))

    cleaned = image.copy()
    out = cleaned.load()
    for x, y in visited:
        out[x, y] = (255, 255, 255, 0)
    return cleaned


def remove_connected_light_background(image: Image.Image) -> Image.Image:
    """Remove edge-connected white matte from design-provided small PNGs."""
    image = image.convert("RGBA")
    width, height = image.size
    pixels = image.load()

    def is_background_candidate(x: int, y: int) -> bool:
        r, g, b, a = pixels[x, y]
        if a == 0:
            return True
        distance_to_white = ((255 - r) ** 2 + (255 - g) ** 2 + (255 - b) ** 2) ** 0.5
        neutral = max(r, g, b) - min(r, g, b) <= 38
        return distance_to_white <= 92 and (neutral or min(r, g, b) >= 220)

    visited: set[tuple[int, int]] = set()
    queue: deque[tuple[int, int]] = deque()
    for x in range(width):
        for y in (0, height - 1):
            if is_background_candidate(x, y):
                visited.add((x, y))
                queue.append((x, y))
    for y in range(height):
        for x in (0, width - 1):
            if is_background_candidate(x, y) and (x, y) not in visited:
                visited.add((x, y))
                queue.append((x, y))

    while queue:
        x, y = queue.popleft()
        for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if 0 <= nx < width and 0 <= ny < height and (nx, ny) not in visited and is_background_candidate(nx, ny):
                visited.add((nx, ny))
                queue.append((nx, ny))

    cleaned = image.copy()
    out = cleaned.load()
    for x, y in visited:
        out[x, y] = (255, 255, 255, 0)
    return cleaned


def remove_edge_connected_neutral_pollution(image: Image.Image) -> Image.Image:
    """Remove edge-connected neutral glow/shadow without touching the icon body."""
    image = image.convert("RGBA")
    width, height = image.size
    pixels = image.load()

    def is_background_candidate(x: int, y: int) -> bool:
        r, g, b, a = pixels[x, y]
        if a == 0:
            return True
        neutral = max(r, g, b) - min(r, g, b) <= 46
        near_white = min(r, g, b) >= 190
        soft_shadow = a <= 120 and neutral
        return near_white or soft_shadow

    visited: set[tuple[int, int]] = set()
    queue: deque[tuple[int, int]] = deque()
    for x in range(width):
        for y in (0, height - 1):
            if is_background_candidate(x, y):
                visited.add((x, y))
                queue.append((x, y))
    for y in range(height):
        for x in (0, width - 1):
            if is_background_candidate(x, y) and (x, y) not in visited:
                visited.add((x, y))
                queue.append((x, y))

    while queue:
        x, y = queue.popleft()
        for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if 0 <= nx < width and 0 <= ny < height and (nx, ny) not in visited and is_background_candidate(nx, ny):
                visited.add((nx, ny))
                queue.append((nx, ny))

    cleaned = image.copy()
    out = cleaned.load()
    for x, y in visited:
        r, g, b, a = out[x, y]
        if a != 0:
            out[x, y] = (r, g, b, 0)
    return cleaned


def prepare_small_assets(source_dir: Path, output_dir: Path) -> None:
    if not source_dir.exists():
        return
    output_dir.mkdir(parents=True, exist_ok=True)
    for size in sorted(SMALL_ASSET_SIZES):
        source = source_dir / f"LocalPinyinIME-{size}.png"
        if not source.exists():
            raise FileNotFoundError(f"missing official small icon source: {source}")
        cleaned = remove_connected_light_background(Image.open(source).convert("RGBA"))
        cleaned = remove_edge_connected_neutral_pollution(cleaned)
        cleaned.save(output_dir / source.name)


def make_large_icon_frame(master: Image.Image, size: int) -> Image.Image:
    crop = master.crop(trim_transparent_bounds(master))

    padding = max(1, round(size * 0.025))
    content_size = max(1, size - padding * 2)
    resized = crop.resize((content_size, content_size), Image.Resampling.LANCZOS)
    frame = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    frame.alpha_composite(resized, (padding, padding))
    if size <= 48:
        frame = frame.filter(ImageFilter.UnsharpMask(radius=0.45, percent=55, threshold=3))
    return remove_edge_connected_neutral_pollution(frame)


def load_exact_small_asset(path: Path, size: int) -> tuple[bytes, Image.Image]:
    if not path.exists():
        raise FileNotFoundError(f"missing official small icon asset: {path}")
    data = path.read_bytes()
    image = Image.open(io.BytesIO(data)).convert("RGBA")
    if image.size != (size, size):
        raise ValueError(f"{path} must be {size}x{size}, got {image.size[0]}x{image.size[1]}")
    return data, image


def encode_png(image: Image.Image) -> bytes:
    png = io.BytesIO()
    image.save(png, format="PNG")
    return png.getvalue()


def encode_icon_dib(image: Image.Image) -> bytes:
    """Encode a 32-bit BGRA DIB icon entry with an explicit alpha-derived AND mask."""
    image = image.convert("RGBA")
    width, height = image.size
    header = struct.pack(
        "<IiiHHIIiiII",
        40,
        width,
        height * 2,
        1,
        32,
        0,
        width * height * 4,
        0,
        0,
        0,
        0,
    )

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


def write_ico(entries: list[tuple[int, bytes]], path: Path) -> None:
    encoded_frames = entries

    header_size = 6 + 16 * len(encoded_frames)
    image_offset = header_size
    directory = bytearray()
    payload = bytearray()
    for size, data in encoded_frames:
        directory.extend(
            struct.pack(
                "<BBBBHHII",
                0 if size == 256 else size,
                0 if size == 256 else size,
                0,
                0,
                1,
                32,
                len(data),
                image_offset,
            )
        )
        payload.extend(data)
        image_offset += len(data)

    path.write_bytes(struct.pack("<HHH", 0, 1, len(encoded_frames)) + directory + payload)


def write_svg_wrapper(master: Image.Image, svg_path: Path, title: str) -> None:
    png = io.BytesIO()
    master.save(png, format="PNG")
    data = base64.b64encode(png.getvalue()).decode("ascii")
    svg_path.write_text(
        "\n".join(
            [
                '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1024 1024" role="img">',
                f"  <title>{title}</title>",
                "  <!-- Development reference only. Production ICO is generated from LocalPinyinIME-master.png. -->",
                f'  <image href="data:image/png;base64,{data}" x="0" y="0" width="1024" height="1024" />',
                "</svg>",
                "",
            ]
        ),
        encoding="utf-8",
    )


def write_contact_sheet(frames: list[Image.Image], path: Path, background: tuple[int, int, int, int]) -> None:
    cell = 288
    label_height = 28
    padding = 20
    width = padding * 2 + cell * len(frames)
    height = padding * 2 + cell + label_height
    sheet = Image.new("RGBA", (width, height), background)
    draw = ImageDraw.Draw(sheet)
    font = ImageFont.load_default()
    text_color = (24, 33, 47, 255) if sum(background[:3]) > 384 else (220, 230, 244, 255)

    for index, frame in enumerate(frames):
        x = padding + index * cell + (cell - frame.width) // 2
        y = padding + (cell - frame.height) // 2
        sheet.alpha_composite(frame, (x, y))

        label = f"{frame.width}px"
        bbox = draw.textbbox((0, 0), label, font=font)
        label_width = bbox[2] - bbox[0]
        draw.text((padding + index * cell + (cell - label_width) // 2, padding + cell + 8),
                  label,
                  fill=text_color,
                  font=font)

    path.parent.mkdir(parents=True, exist_ok=True)
    sheet.convert("RGB").save(path)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, default=Path("assets/branding/icon"))
    parser.add_argument("--master", type=Path)
    parser.add_argument("--small-assets-dir", type=Path)
    parser.add_argument("--small-source-dir", type=Path)
    parser.add_argument("--contact-sheet", type=Path)
    parser.add_argument("--light-contact-sheet", type=Path)
    parser.add_argument("--dark-contact-sheet", type=Path)
    args = parser.parse_args()

    output_dir = args.output_dir
    output_dir.mkdir(parents=True, exist_ok=True)
    master_path = args.master or output_dir / "LocalPinyinIME-master.png"
    if not master_path.exists():
        raise FileNotFoundError(f"missing approved PNG master: {master_path}")
    small_assets_dir = args.small_assets_dir or output_dir / "small"
    small_source_dir = args.small_source_dir or small_assets_dir / "source"
    prepare_small_assets(small_source_dir, small_assets_dir)

    master = remove_preview_checkerboard(Image.open(master_path).convert("RGBA"))
    master = remove_edge_connected_neutral_pollution(master)
    entries: list[tuple[int, bytes]] = []
    frames: list[Image.Image] = []
    for size in ICO_SIZES:
        if size in SMALL_ASSET_SIZES:
            png_data, frame = load_exact_small_asset(small_assets_dir / f"LocalPinyinIME-{size}.png", size)
            entries.append((size, encode_icon_dib(frame)))
            frames.append(frame)
        else:
            frame = make_large_icon_frame(master, size)
            entries.append((size, encode_icon_dib(frame)))
            frames.append(frame)

    write_ico(entries, output_dir / "LocalPinyinIME.ico")
    write_svg_wrapper(master, output_dir / "LocalPinyinIME.svg", "LocalPinyinIME Concept A icon")
    write_svg_wrapper(master, output_dir / "LocalPinyinIME-small.svg", "LocalPinyinIME Concept A small icon")
    if args.contact_sheet:
        write_contact_sheet(frames, args.contact_sheet, (248, 250, 252, 255))
    if args.light_contact_sheet:
        write_contact_sheet(frames, args.light_contact_sheet, (248, 250, 252, 255))
    if args.dark_contact_sheet:
        write_contact_sheet(frames, args.dark_contact_sheet, (15, 23, 42, 255))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
