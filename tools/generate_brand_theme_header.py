#!/usr/bin/env python3
import argparse
import json
from pathlib import Path


def cpp_identifier(*parts: str) -> str:
    return "kLocalPinyinBlue" + "".join(part[:1].upper() + part[1:] for part in parts)


def parse_color(value: str) -> tuple[int, int, int]:
    if not isinstance(value, str) or len(value) != 7 or not value.startswith("#"):
        raise ValueError(f"invalid color value: {value!r}")
    return int(value[1:3], 16), int(value[3:5], 16), int(value[5:7], 16)


def emit_color(name: str, value: str) -> str:
    red, green, blue = parse_color(value)
    return f"inline constexpr COLORREF {name} = RGB(0x{red:02X}, 0x{green:02X}, 0x{blue:02X});"


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate the LocalPinyinIME brand theme C++ header.")
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    data = json.loads(args.input.read_text(encoding="utf-8"))
    colors = data["colors"]
    surfaces = data["surfaces"]
    text = data["text"]
    states = data["states"]

    lines = [
        "#pragma once",
        "",
        "#include <windows.h>",
        "",
        "namespace localpinyin::brand {",
        "",
    ]

    for key in ["deep_blue", "azure_blue", "cyan_highlight", "cool_gray", "near_white"]:
        lines.append(emit_color(cpp_identifier(key), colors[key]))
    lines.append("")

    for mode in ["light", "dark"]:
        for key, value in surfaces[mode].items():
            lines.append(emit_color(cpp_identifier(mode, key), value))
        for key, value in text[mode].items():
            lines.append(emit_color(cpp_identifier(mode, key), value))
        for key, value in states[mode].items():
            lines.append(emit_color(cpp_identifier(mode, key), value))
        lines.append("")

    lines.append("}  // namespace localpinyin::brand")
    lines.append("")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
